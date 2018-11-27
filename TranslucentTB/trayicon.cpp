#include "trayicon.hpp"
#include <shellapi.h>

#include "common.hpp"
#include "ttberror.hpp"
#include "ttblog.hpp"
#include "util.hpp"
#include "win32.hpp"

bool TrayIcon::IsSystemLightThemeEnabled()
{
	static const bool isLightThemeAvailable = win32::IsAtLeastBuild(MIN_LIGHT_BUILD);

	if (isLightThemeAvailable)
	{
		DWORD value;
		DWORD size = sizeof(value);
		LRESULT error = RegGetValue(HKEY_CURRENT_USER, LR"(SOFTWARE\Microsoft\Windows\CurrentVersion\Themes\Personalize)", L"SystemUsesLightTheme", RRF_RT_REG_DWORD, NULL, &value, &size);
		switch (error)
		{
		case ERROR_SUCCESS:
			return value != 0;

		case ERROR_FILE_NOT_FOUND:
			return false;

		default:
			ErrorHandle(HRESULT_FROM_WIN32(error), Error::Level::Log, L"Failed to query light system theme state.");
			return false;
		}
	}
	else
	{
		return false;
	}
}

long TrayIcon::RegisterIcon(...)
{
	UpdateIcon();
	if (!Shell_NotifyIcon(NIM_ADD, &m_IconData))
	{
		Log::OutputMessage(L"Failed to notify shell of icon addition.");
	}
	if (!Shell_NotifyIcon(NIM_SETVERSION, &m_IconData))
	{
		Log::OutputMessage(L"Failed to notify shell of icon version.");
	}

	return 0;
}

long TrayIcon::UpdateIcon(bool notify, ...)
{
	DestroyIconHandle();
	ErrorHandle(
		LoadIconMetric(m_hInstance, IsSystemLightThemeEnabled() ? m_DarkIconResource : m_BrightIconResource, LIM_SMALL, &m_IconData.hIcon),
		Error::Level::Log,
		L"Failed to load tray icon."
	);

	if (notify && !Shell_NotifyIcon(NIM_MODIFY, &m_IconData))
	{
		Log::OutputMessage(L"Failed to notify shell of icon modification.");
	}

	return 0;
}

void TrayIcon::DestroyIconHandle()
{
	if (m_IconData.hIcon && !DestroyIcon(m_IconData.hIcon))
	{
		LastErrorHandle(Error::Level::Log, L"Failed to destroy tray icon.");
	}
}

TrayIcon::TrayIcon(MessageWindow &window, const wchar_t *brightIconResource, const wchar_t *darkIconResource, unsigned int additionalFlags, HINSTANCE hInstance) :
	m_Window(window),
	m_IconData {
		sizeof(m_IconData),
		m_Window,
		LOWORD(brightIconResource),
		NIF_ICON | NIF_TIP | NIF_MESSAGE | additionalFlags
	},
	m_hInstance(hInstance),
	m_BrightIconResource(brightIconResource),
	m_DarkIconResource(darkIconResource)
{
	wcscpy_s(m_IconData.szTip, NAME);

	m_IconData.uCallbackMessage = Util::GetRandomNumber<unsigned int>(WM_APP, 0xBFFF);

	RegisterIcon();

	m_TaskbarCreatedCookie = m_Window.RegisterCallback(WM_TASKBARCREATED, std::bind(&TrayIcon::RegisterIcon, this));
	m_SettingsChangedCookie = m_Window.RegisterCallback(WM_SETTINGCHANGE, std::bind(&TrayIcon::UpdateIcon, this, true));
}

TrayIcon::~TrayIcon()
{
	if (!Shell_NotifyIcon(NIM_DELETE, &m_IconData))
	{
		Log::OutputMessage(L"Failed to notify shell of icon deletion.");
	}
	m_Window.UnregisterCallback(m_TaskbarCreatedCookie);
	m_Window.UnregisterCallback(m_SettingsChangedCookie);
	DestroyIconHandle();
}