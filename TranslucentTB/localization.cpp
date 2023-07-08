#include "localization.hpp"
#include <libloaderapi.h>
#include <WinBase.h>
#include <WinUser.h>
#include "winrt.hpp"
#include <winrt/Windows.ApplicationModel.Resources.Core.h>

#include "../ProgramLog/error/win32.hpp"
#include "../ProgramLog/error/winrt.hpp"

bool Localization::SetProcessLangOverride(std::wstring_view langOverride)
{
	// make double null terminated
	std::wstring language(langOverride);
	language.push_back(L'\0');

	// set for process
	if (!SetProcessPreferredUILanguages(MUI_LANGUAGE_NAME, language.c_str(), nullptr))
	{
		LastErrorHandle(spdlog::level::err, L"Failed to set process UI language. Is the language set in the configuration file a BCP-47 language name?");

		// don't try setting thread & XAML language, it'll probably fail too
		return false;
	}

	// SetProcessPreferredUILanguages does not affect the lookup behavior of resource functions like FindResourceEx,
	// only SetThreadPreferredUILanguages does.
	// WHY WINDOWS
	// WHAT IS THE POINT OF SETPROCESSPREFERREDUILANGUAGES THEN
	if (!SetThreadPreferredUILanguages(MUI_LANGUAGE_NAME, language.c_str(), nullptr))
	{
		LastErrorHandle(spdlog::level::err, L"Failed to set thread UI language. Is the language set in the configuration file a BCP-47 language name?");

		// remove the existing override to not fail in a partially localized to previous value state
		SetProcessPreferredUILanguages(MUI_LANGUAGE_NAME, nullptr, nullptr);

		// don't try setting XAML language, it'll probably fail too
		return false;
	}

	// set for WinRT resources
	try
	{
		winrt::Windows::ApplicationModel::Resources::Core::ResourceContext::SetGlobalQualifierValue(L"Language", langOverride);
	}
	catch (const winrt::hresult_error& err)
	{
		HresultErrorHandle(err, spdlog::level::err, L"Failed to set resource language override. Is the language set in the configuration file a BCP-47 language name?");

		// remove the existing overrides to not fail in a partially localized to previous value state
		SetThreadPreferredUILanguages(MUI_LANGUAGE_NAME, nullptr, nullptr);
		SetProcessPreferredUILanguages(MUI_LANGUAGE_NAME, nullptr, nullptr);

		return false;
	}

	return true;
}

Util::null_terminated_wstring_view Localization::LoadLocalizedResourceString(uint16_t resource, HINSTANCE hInst, WORD lang)
{
	const auto fail = [resource, hInst, lang]() -> Util::null_terminated_wstring_view
	{
		if (lang != MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US))
		{
			// try again in English
			return LoadLocalizedResourceString(resource, hInst, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
		}
		else
		{
			return L"[error occurred while loading localized string]";
		}
	};

	const HRSRC src = FindResourceEx(hInst, RT_STRING, MAKEINTRESOURCE((resource >> 4) + 1), lang);
	if (!src)
	{
		LastErrorHandle(spdlog::level::warn, L"Failed to find string resource.");
		return fail();
	}

	const HGLOBAL res = LoadResource(hInst, src);
	if (!res)
	{
		LastErrorHandle(spdlog::level::warn, L"Failed to load string resource.");
		return fail();
	}

	auto str = static_cast<const wchar_t *>(LockResource(res));
	if (!str)
	{
		LastErrorHandle(spdlog::level::warn, L"Failed to lock string resource.");
		return fail();
	}

	for (int i = 0; i < (resource & 0xF); i++)
	{
		str += 1 + static_cast<uint16_t>(*str);
	}

	std::wstring_view resStr { str + 1, static_cast<uint16_t>(*str) };
	if (!resStr.empty() && resStr.back() == L'\0')
	{
		return Util::null_terminated_wstring_view::make_unsafe(resStr.data(), resStr.length() - 1);
	}
	else
	{
		return fail();
	}
}

std::thread Localization::ShowLocalizedMessageBox(uint16_t resource, UINT type, HINSTANCE hInst, WORD lang)
{
	const auto msg = LoadLocalizedResourceString(resource, hInst, lang);

	return std::thread([msg, type, lang]() noexcept
	{
		MessageBoxEx(Window::NullWindow, msg.c_str(), APP_NAME, type, lang);
	});
}
