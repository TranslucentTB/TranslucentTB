#include "trayicon.hpp"
#include <shellapi.h>

#include "appinfo.hpp"
#include "constants.hpp"
#include "undoc/dynamicloader.hpp"
#include "../../ProgramLog/error/errno.hpp"
#include "../../ProgramLog/error/std.hpp"
#include "../../ProgramLog/error/win32.hpp"

void TrayIcon::LoadThemedIcon()
{
	const wchar_t *icon = m_whiteIconResource;
	if (const auto ssudm = DynamicLoader::ShouldSystemUseDarkMode(); ssudm && !ssudm())
	{
		icon = m_darkIconResource;
	}

	if (const HRESULT hr = LoadIconMetric(hinstance(), icon, LIM_SMALL, m_Icon.put()); SUCCEEDED(hr))
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
			LastErrorHandle(spdlog::level::info, L"Failed to notify shell.");
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
		LoadThemedIcon();

		if (m_CurrentlyShowing)
		{
			Notify(NIM_MODIFY);
		}
		break;
	default:
		if (uMsg == m_TaskbarCreatedMessage)
		{
			if (m_ShowPreference)
			{
				// it's not actually showing anymore, explorer restarted
				m_CurrentlyShowing = false;

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
		.uFlags = NIF_MESSAGE | NIF_GUID,
		.uCallbackMessage = TRAY_CALLBACK,
		.uVersion = NOTIFYICON_VERSION_4,
		.guidItem = iconId
	},
	m_whiteIconResource(whiteIconResource),
	m_darkIconResource(darkIconResource),
	m_ShowPreference(false),
	m_CurrentlyShowing(false),
	m_TaskbarCreatedMessage(Window::RegisterMessage(WM_TASKBARCREATED))
{
	if (const errno_t err = wcscpy_s(m_IconData.szTip, windowName.c_str()); !err)
	{
		m_IconData.uFlags |= NIF_TIP | NIF_SHOWTIP;
	}
	else
	{
		ErrnoTHandle(err, spdlog::level::warn, L"Failed to copy tray icon tooltip text.");
	}

	LoadThemedIcon();

	// Clear icon from a previous instance that didn't cleanly exit.
	// Errors if instance cleanly exited, so avoid logging it.
	Notify(NIM_DELETE, true);
}

void TrayIcon::Show()
{
	m_ShowPreference = true;
	if (!m_CurrentlyShowing && Notify(NIM_ADD))
	{
		Notify(NIM_SETVERSION);
		m_CurrentlyShowing = true;
	}
}

void TrayIcon::Hide()
{
	m_ShowPreference = false;
	if (m_CurrentlyShowing && Notify(NIM_DELETE))
	{
		m_CurrentlyShowing = false;
	}
}

TrayIcon::~TrayIcon()
{
	Hide();
}
