#include "trayicon.hpp"
#include <shellapi.h>

#include "appinfo.hpp"
#include "constants.hpp"
#include "undoc/dynamicloader.hpp"
#include "../../ProgramLog/error/std.hpp"
#include "../../ProgramLog/error/win32.hpp"

void TrayIcon::LoadThemedIcon()
{
	const wchar_t *icon = m_whiteIconResource;
	if (DynamicLoader::uxtheme())
	{
		if (const auto ssudm = DynamicLoader::ShouldSystemUseDarkMode(); ssudm && !ssudm())
		{
			icon = m_darkIconResource;
		}
	}

	if (HRESULT hr = LoadIconMetric(hinstance(), icon, LIM_SMALL, m_Icon.put()); SUCCEEDED(hr))
	{
		m_IconData.uFlags |= NIF_ICON;
		m_IconData.hIcon = m_Icon.get();
	}
	else
	{
		m_IconData.uFlags &= ~NIF_ICON;
		m_Icon.reset();
		HresultHandle(hr, spdlog::level::warn, L"Failed to load tray icon.");
	}
}

bool TrayIcon::Notify(DWORD message, bool ignoreError)
{
	if (Shell_NotifyIcon(message, &m_IconData))
	{
		return true;
	}
	else
	{
		if (!ignoreError)
		{
			LastErrorHandle(spdlog::level::warn, L"Failed to notify shell.");
		}

		return false;
	}
}

LRESULT TrayIcon::MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_DPICHANGED:
	case WM_SETTINGCHANGE:
		if (m_Show)
		{
			LoadThemedIcon();
			Notify(NIM_MODIFY);
		}
		break;
	default:
		if (m_TaskbarCreatedMessage && uMsg == *m_TaskbarCreatedMessage)
		{
			if (m_Show)
			{
				m_Show = false;
				Show();
			}

			return 0;
		}
	}

	return MessageWindow::MessageHandler(uMsg, wParam, lParam);
}

TrayIcon::TrayIcon(const GUID &iconId, Util::null_terminated_wstring_view className,
	Util::null_terminated_wstring_view windowName, const wchar_t *whiteIconResource,
	const wchar_t *darkIconResource, HINSTANCE hInstance) :
	MessageWindow(className, windowName, hInstance),
	m_IconData {
		.cbSize = sizeof(m_IconData),
		.hWnd = m_WindowHandle,
		.uFlags = NIF_TIP | NIF_MESSAGE | NIF_GUID,
		.uCallbackMessage = TRAY_CALLBACK,
		.uVersion = NOTIFYICON_VERSION_4,
		.guidItem = iconId
	},
	m_whiteIconResource(whiteIconResource),
	m_darkIconResource(darkIconResource),
	m_Show(false),
	m_TaskbarCreatedMessage(Window::RegisterMessage(WM_TASKBARCREATED))
{
	if (const errno_t err = wcscpy_s(m_IconData.szTip, windowName.c_str()))
	{
		ErrnoTHandle(err, spdlog::level::warn, L"Failed to copy tray icon tooltip text.");
		m_IconData.uFlags &= ~NIF_TIP;
	}

	// Clear icon from a previous instance that didn't cleanly exit.
	// Errors if instance cleanly exited, so avoid logging it.
	Notify(NIM_DELETE, true);
}

void TrayIcon::Show()
{
	if (!m_Show)
	{
		if (!m_Icon)
		{
			LoadThemedIcon();
		}

		if (Notify(NIM_ADD) && Notify(NIM_SETVERSION))
		{
			m_Show = true;
		}
	}
}

void TrayIcon::Hide()
{
	if (m_Show)
	{
		if (Notify(NIM_DELETE))
		{
			m_Icon.reset();
			m_Show = false;
		}
	}
}

TrayIcon::~TrayIcon()
{
	Hide();
}
