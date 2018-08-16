#include "trayicon.hpp"
#include <shellapi.h>

#include "common.hpp"
#include "util.hpp"
#include "ttberror.hpp"
#include "ttblog.hpp"

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

TrayIcon::TrayIcon(MessageWindow &window, wchar_t *iconResource, const unsigned int additionalFlags, const HINSTANCE &hInstance) :
	m_Window(window),
	m_IconData {
		sizeof(m_IconData),
		m_Window,
		LOWORD(iconResource),
		NIF_ICON | NIF_TIP | NIF_MESSAGE | additionalFlags
	}
{
	ErrorHandle(LoadIconMetric(hInstance, iconResource, LIM_SMALL, &m_IconData.hIcon), Error::Level::Log, L"Failed to load tray icon.");
	wcscpy_s(m_IconData.szTip, NAME);

	m_IconData.uCallbackMessage = Util::GetRandomNumber<unsigned int>(WM_APP, 0xBFFF);

	RegisterIcon();

	m_Cookie = m_Window.RegisterCallback(WM_TASKBARCREATED, std::bind(&TrayIcon::RegisterIcon, this));
}

TrayIcon::~TrayIcon()
{
	if (!Shell_NotifyIcon(NIM_DELETE, &m_IconData))
	{
		Log::OutputMessage(L"Failed to notify shell of icon deletion.");
	}
	m_Window.UnregisterCallback(m_Cookie);
	if (!DestroyIcon(m_IconData.hIcon))
	{
		LastErrorHandle(Error::Level::Log, L"Failed to destroy tray icon.");
	}
}