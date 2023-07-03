#include "localization.hpp"
#include <libloaderapi.h>
#include <WinBase.h>
#include <WinUser.h>

#include "../ProgramLog/error/win32.hpp"

Util::null_terminated_wstring_view Localization::LoadLocalizedResourceString(uint16_t resource, HINSTANCE hInst, WORD lang)
{
	const HRSRC src = FindResourceEx(hInst, RT_STRING, MAKEINTRESOURCE((resource >> 4) + 1), lang);
	if (!src)
	{
		LastErrorHandle(spdlog::level::warn, L"Failed to find string resource.");
		return FAILED_LOADING;
	}

	const HGLOBAL res = LoadResource(hInst, src);
	if (!res)
	{
		LastErrorHandle(spdlog::level::warn, L"Failed to load string resource.");
		return FAILED_LOADING;
	}

	auto str = static_cast<const wchar_t *>(LockResource(res));
	if (!str)
	{
		LastErrorHandle(spdlog::level::warn, L"Failed to lock string resource.");
		return FAILED_LOADING;
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
	else if (lang != MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US))
	{
		// try again in English
		return LoadLocalizedResourceString(resource, hInst, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
	}
	else
	{
		return FAILED_LOADING;
	}
}
