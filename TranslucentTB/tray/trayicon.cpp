#include "trayicon.hpp"
#include <shellapi.h>

#include "appinfo.hpp"
#include "constants.hpp"
#include "../../ProgramLog/error.hpp"
#include "util/random.hpp"
#include "win32.hpp"

void TrayIcon::LoadIcon()
{
	HresultHandle(
		LoadIconMetric(m_hInstance, m_IconResource, LIM_SMALL, m_Icon.put()),
		spdlog::level::warn,
		L"Failed to load tray icon."
	);

	m_IconData.hIcon = m_Icon.get();
}

long TrayIcon::UpdateIcon(...)
{
	LoadIcon();

	if (m_Show)
	{
		if (!Shell_NotifyIcon(NIM_MODIFY, &m_IconData))
		{
			MessagePrint(spdlog::level::warn, L"Failed to notify shell of icon modification.");
		}
	}

	return 0;
}

TrayIcon::TrayIcon(MessageWindow &window, const wchar_t *iconResource, bool hide, HINSTANCE hInstance, unsigned int additionalFlags) :
	m_Window(window),
	m_IconData {
		.cbSize = sizeof(m_IconData),
		.hWnd = m_Window,
		.uID = LOWORD(iconResource), // todo: not use an UID
		.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | additionalFlags,
		.uCallbackMessage = Util::GetRandomNumber<unsigned int>(WM_APP, 0xBFFF),
		.szTip = APP_NAME
	},
	m_hInstance(hInstance),
	m_IconResource(iconResource),
	m_Show(!hide)
{
	LoadIcon();
	if (m_Show)
	{
		Show();
	}

	m_TaskbarCreatedCookie = m_Window.RegisterCallback(WM_TASKBARCREATED, [this](...)
	{
		if (m_Show)
		{
			Show();
		}

		return 0;
	});
	m_DpiChangedCookie = m_Window.RegisterCallback(WM_DPICHANGED, std::bind(&TrayIcon::UpdateIcon, this));
}

void TrayIcon::Show()
{
	if (!Shell_NotifyIcon(NIM_ADD, &m_IconData))
	{
		MessagePrint(spdlog::level::warn, L"Failed to notify shell of icon addition.");
		return;
	}
	if (!Shell_NotifyIcon(NIM_SETVERSION, &m_IconData))
	{
		MessagePrint(spdlog::level::warn, L"Failed to notify shell of icon version.");
		return;
	}

	m_Show = true;
}

void TrayIcon::Hide()
{
	if (!Shell_NotifyIcon(NIM_DELETE, &m_IconData))
	{
		MessagePrint(spdlog::level::info, L"Failed to notify shell of icon deletion.");
	}

	m_Show = false;
}

TrayIcon::~TrayIcon()
{
	if (m_Show)
	{
		Hide();
	}

	m_Window.UnregisterCallback(m_TaskbarCreatedCookie);
	m_Window.UnregisterCallback(m_DpiChangedCookie);
}
