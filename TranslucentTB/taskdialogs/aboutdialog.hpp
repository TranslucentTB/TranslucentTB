#pragma once
#include "taskdialog.hpp"
#include <detours/detours.h>
#include <fmt/format.h>
#include <iterator>
#include <rapidjson/rapidjson.h>
#include <spdlog/version.h>
#include <sstream>
#include <wil/safecast.h>

#include "appinfo.hpp"
#include "../../ProgramLog/error.hpp"
#include "util/numbers.hpp"
#include "../uwp/uwp.hpp"
#include "uwp.hpp"
#include "win32.hpp"

class AboutDialog : public TTBTaskDialog {
private:
	static constexpr int COPY_VERSION = 40000;
	static constexpr int JOIN_DISCORD = COPY_VERSION + 1;
	static constexpr int DONATE = JOIN_DISCORD + 1;

	static constexpr TASKDIALOG_BUTTON s_Buttons[] = {
		{
			COPY_VERSION,
			L"Copy system info to clipboard\nUse this when filling a GitHub bug report."
		},
		{
			JOIN_DISCORD,
			L"Join our Discord server\nChat with the community and developers."
		},
		{
			DONATE,
			L"Donate\nSupport us developing " APP_NAME " and bringing other great features to you!"
		}
	};

	inline HRESULT CallbackProc(Window window, unsigned int uNotification, WPARAM wParam, LPARAM)
	{
		if (uNotification == TDN_BUTTON_CLICKED)
		{
			switch (wParam)
			{
			case COPY_VERSION:
				try
				{
					UWP::CopyToClipboard(BuildVersionInfo());
					MessageBoxEx(window, L"Copied.", APP_NAME, MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND, MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL));
				}
				HresultErrorCatch(spdlog::level::err, L"Failed to copy version information!")
				return S_FALSE;

			// TODO: update (or not since taskdialogs are going away lul)
			case JOIN_DISCORD:
				win32::OpenLink(L"https://discord.gg/w95DGTK");
				break;

			case DONATE:
				win32::OpenLink(L"https://liberapay.com/" APP_NAME);
				break;
			}
		}
		return S_OK;
	}

	inline static std::wstring BuildVersionInfo()
	{
		// TODO: remove stream (or not since task dialogs are going away)
		std::wostringstream str;

		str << L"Build configuration: "
#if defined(NDEBUG)
			L"Release"
#elif defined(_DEBUG)
			L"Debug"
#else
			L"Unknown"
#endif
			L" ("
#if defined(_AMD64_)
			L"x64"
#elif defined (_X86_)
			L"x86"
#elif defined(_ARM64_)
			L"ARM64"
#elif defined(_ARM_)
			L"ARM"
#else
			L"Unknown"
#endif
			L")\n"

		L"System architecture: " << win32::GetProcessorArchitecture() << std::endl;

		if (UWP::HasPackageIdentity())
		{
			str << L"Package version: ";
			try
			{
				str << UWP::GetApplicationVersion().ToString();
			}
			catch (const winrt::hresult_error &error)
			{
				str << Error::MessageFromHresultError(error);
			}
			str << std::endl;
		}

		str << APP_NAME L" version: " APP_VERSION << std::endl;

		const auto [build, hr2] = win32::GetWindowsBuild();
		str << L"Windows version: " << (SUCCEEDED(hr2) ? build : Error::MessageFromHRESULT(hr2)) << std::endl;

		const uint8_t major = (DETOURS_VERSION & 0xf0000) >> 16;
		const uint8_t minor = (DETOURS_VERSION & 0xf00) >> 8;
		const uint8_t revision = DETOURS_VERSION & 0xf;
		str << L"Microsoft Detours version: " << major << L'.' << minor << L'.' << revision << std::endl;

		str << L"RapidJSON version: " << RAPIDJSON_VERSION_STRING << std::endl;
		str << L"spdlog version: " << SPDLOG_VER_MAJOR << L'.' << SPDLOG_VER_MINOR << L'.' << SPDLOG_VER_PATCH << std::endl;
		str << L"{fmt} version: " << std::floor(FMT_VERSION / 10000) << L'.' << std::floor(FMT_VERSION % 10000 / 100) << L'.' << (FMT_VERSION % 100);

		return str.str();
	}

	inline static std::wstring BuildAboutContent()
	{
		return fmt::format(
			fmt(L"This program is free (as in freedom) software, redistributed under the GPLv3. "
			LR"(As such, the <A HREF="https://github.com/)" APP_NAME L"/" APP_NAME LR"(">source code</A> is available for anyone to modify, inspect, compile, etc...)" L"\n\n"
			LR"(Brought to you by <A HREF="https://github.com/)" APP_NAME L"/" APP_NAME LR"(/graphs/contributors">all its contributors</A>.)" L"\n\n{}\n\n"
			L"All trademarks, product names, company names, logos, service marks, copyrights and/or trade dress mentioned, displayed, cited, or otherwise indicated are the property of their respective owners."),
			BuildVersionInfo()
		);
	}

public:
	inline AboutDialog() :
		TTBTaskDialog(
			L"About " APP_NAME,
			BuildAboutContent(),
			std::bind(&AboutDialog::CallbackProc, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4),
			Window::NullWindow
		)
	{
		m_Cfg.dwFlags |= TDF_ALLOW_DIALOG_CANCELLATION | TDF_USE_COMMAND_LINKS;
		m_Cfg.dwCommonButtons = TDCBF_CLOSE_BUTTON;

		m_Cfg.cButtons = wil::safe_cast<UINT>(std::size(s_Buttons));
		m_Cfg.pButtons = s_Buttons;
	}

	inline void Run()
	{
		bool _;
		TTBTaskDialog::Run(_);
	}
};
