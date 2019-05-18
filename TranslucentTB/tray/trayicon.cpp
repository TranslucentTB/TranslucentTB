#include "trayicon.hpp"
#include <shellapi.h>

#include "constants.hpp"
#include "../ttberror.hpp"
#include "../ttblog.hpp"
#include "util/random.hpp"
#include "../win32.hpp"

long TrayIcon::RegisterIcon(...)
{
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

void TrayIcon::LoadIcon()
{
	ErrorHandle(
		LoadIconMetric(m_hInstance, m_IconResource, LIM_SMALL, m_Icon.put()),
		Error::Level::Log,
		L"Failed to load tray icon."
	);

	m_IconData.hIcon = m_Icon.get();
}

long TrayIcon::UpdateIcon(...)
{
	LoadIcon();

	if (!Shell_NotifyIcon(NIM_MODIFY, &m_IconData))
	{
		Log::OutputMessage(L"Failed to notify shell of icon modification.");
	}

	return 0;
}

TrayIcon::TrayIcon(MessageWindow &window, const wchar_t *iconResource, unsigned int additionalFlags, HINSTANCE hInstance) :
	m_Window(window),
	m_IconData {
		.cbSize = sizeof(m_IconData),
		.hWnd = m_Window,
		.uID = LOWORD(iconResource), // todo: not use an UID
		.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | additionalFlags,
		.uCallbackMessage = Util::GetRandomNumber<unsigned int>(WM_APP, 0xBFFF),
		.szTip = NAME
	},
	m_hInstance(hInstance),
	m_IconResource(iconResource)
{
	LoadIcon();
	RegisterIcon();

	m_TaskbarCreatedCookie = m_Window.RegisterCallback(WM_TASKBARCREATED, std::bind(&TrayIcon::RegisterIcon, this));
	m_DpiChangedCookie = m_Window.RegisterCallback(WM_DPICHANGED, std::bind(&TrayIcon::UpdateIcon, this));
}

TrayIcon::~TrayIcon()
{
	if (!Shell_NotifyIcon(NIM_DELETE, &m_IconData))
	{
		Log::OutputMessage(L"Failed to notify shell of icon deletion.");
	}
	m_Window.UnregisterCallback(m_TaskbarCreatedCookie);
	m_Window.UnregisterCallback(m_DpiChangedCookie);
}