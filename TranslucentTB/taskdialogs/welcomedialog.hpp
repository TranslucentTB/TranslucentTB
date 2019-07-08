#pragma once
#include "taskdialog.hpp"
#include <filesystem>
#include <fmt/format.h>

#include "constants.hpp"
#include "ttberror.hpp"

class WelcomeDialog : public TTBTaskDialog {
private:
	inline HRESULT CallbackProc(Window window, unsigned int uNotification, WPARAM wParam, LPARAM)
	{
		switch (uNotification)
		{
		case TDN_VERIFICATION_CLICKED:
			window.send_message(TDM_ENABLE_BUTTON, IDOK, wParam);
			break;

		case TDN_CREATED:
			window.send_message(TDM_CLICK_VERIFICATION, FALSE, FALSE);
			break;

		case TDN_BUTTON_CLICKED:
			if (wParam == IDCANCEL)
			{
				window.send_message(TDM_CLICK_VERIFICATION, FALSE, FALSE);
			}
		}

		return S_OK;
	}

	inline std::wstring BuildWelcomeContent(const std::filesystem::path &configLocation)
	{
		return fmt::format(
			LR"(All the settings for the application can be edited from the tray icon. If you want to edit the raw configuration file (in JSON), take a look at <A HREF="{0}">{0}</A>. )"
			L"All changes are automatically reloaded. If you prefer a command line, run " APP_NAME L" with the --help command line argument to get more info.\n\n"
			L"If you appreciate " APP_NAME LR"( you are more than welcome to <A HREF="https://liberapay.com/)" APP_NAME LR"(">donate</A>.)" L"\n\n"
			L"You must agree to our license, the GPLv3, before using " APP_NAME L". We will only ask this once. Check the box and press OK to continue.",
			configLocation.native()
		);
	}

public:
	inline WelcomeDialog(const std::filesystem::path &configLocation) :
		TTBTaskDialog(
			L"Welcome to " APP_NAME L"!",
			BuildWelcomeContent(configLocation),
			std::bind(&WelcomeDialog::CallbackProc, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4),
			Window::NullWindow
		)
	{
		m_Cfg.pszVerificationText = L"I agree to the terms of the GPLv3.";
		m_Cfg.dwCommonButtons = TDCBF_OK_BUTTON | TDCBF_CANCEL_BUTTON;

		m_Cfg.pszFooter = LR"(You can read our license <A HREF="https://github.com/)" APP_NAME L"/" APP_NAME LR"(/blob/master/LICENSE.md">here</A>.)";
		m_Cfg.dwFlags |= TDF_USE_HICON_FOOTER;
		LoadIconMetric(nullptr, IDI_INFORMATION, LIM_SMALL, &m_Cfg.hFooterIcon);
	}

	inline bool Run()
	{
		bool confirm = false;
		return TTBTaskDialog::Run(confirm) && confirm;
	}

	inline ~WelcomeDialog()
	{
		if (m_Cfg.hFooterIcon && !DestroyIcon(m_Cfg.hFooterIcon))
		{
			LastErrorHandle(spdlog::level::info, L"Failed to destroy dialog icon.");
		}
	}
};