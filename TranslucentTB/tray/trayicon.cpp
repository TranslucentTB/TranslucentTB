#include "trayicon.hpp"
#include <cguid.h>
#include <shellapi.h>

#include "appinfo.hpp"
#include "constants.hpp"
#include "../localization.hpp"
#include "../../ProgramLog/error/errno.hpp"
#include "../../ProgramLog/error/std.hpp"
#include "../../ProgramLog/error/win32.hpp"
#include "util/color.hpp"

const wchar_t *TrayIcon::GetThemedIcon() const
{
	HIGHCONTRAST info = { .cbSize = sizeof(info) };
	if (SystemParametersInfo(SPI_GETHIGHCONTRAST, 0, &info, 0))
	{
		if (info.dwFlags & HCF_HIGHCONTRASTON)
		{
			return Util::Color::FromABGR(GetSysColor(COLOR_WINDOWTEXT)).IsDarkColor()
				? m_darkIconResource : m_whiteIconResource;
		}
	}
	else
	{
		LastErrorHandle(spdlog::level::info, L"Failed to check if high contrast mode is enabled");
	}

	return m_Ssudm && !m_Ssudm() ? m_darkIconResource : m_whiteIconResource;
}

void TrayIcon::LoadThemedIcon()
{
	if (const HRESULT hr = LoadIconMetric(hinstance(), GetThemedIcon(), LIM_SMALL, m_Icon.put()); SUCCEEDED(hr))
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

bool TrayIcon::Notify(DWORD message, NOTIFYICONDATA *data)
{
	if (Shell_NotifyIcon(message, data ? data : &m_IconData))
	{
		return true;
	}
	else
	{
		MessagePrint(spdlog::level::info, L"Failed to notify shell.");

		return false;
	}
}

LRESULT TrayIcon::MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_DPICHANGED:
	case WM_SETTINGCHANGE:
	case WM_THEMECHANGED:
	case WM_DISPLAYCHANGE:
		LoadThemedIcon();

		if (m_CurrentlyShowing)
		{
			Notify(NIM_MODIFY);
		}
		break;
	default:
		if (uMsg == m_TaskbarCreatedMessage)
		{
			LoadThemedIcon();

			// https://github.com/TranslucentTB/TranslucentTB/issues/417
			// we also get this message when DPI changes. Check if explorer truly
			// restarted by seeing if a normal NIM_MODIFY worked
			m_CurrentlyShowing = Shell_NotifyIcon(NIM_MODIFY, &m_IconData);

			if (m_ShowPreference)
			{
				Show();
			}

			return 0;
		}
	}

	return MessageWindow::MessageHandler(uMsg, wParam, lParam);
}

std::optional<RECT> TrayIcon::GetTrayRect()
{
	NOTIFYICONIDENTIFIER id = { sizeof(id) };
#ifdef SIGNED_BUILD
	id.guidItem = m_IconData.guidItem;
#else
	id.hWnd = m_IconData.hWnd;
	id.uID = m_IconData.uID;
	id.guidItem = GUID_NULL;
#endif

	RECT rect{};
	const HRESULT hr = Shell_NotifyIconGetRect(&id, &rect);
	if (SUCCEEDED(hr))
	{
		return rect;
	}
	else
	{
		HresultHandle(hr, spdlog::level::warn, L"Failed to get tray rect");
		return std::nullopt;
	}
}

TrayIcon::TrayIcon(const GUID &iconId, const wchar_t *whiteIconResource,
	const wchar_t *darkIconResource, PFN_SHOULD_SYSTEM_USE_DARK_MODE ssudm) :
	m_IconData {
		.cbSize = sizeof(m_IconData),
		.hWnd = m_WindowHandle,
		.uFlags = NIF_MESSAGE | NIF_TIP,
		.uCallbackMessage = TRAY_CALLBACK,
		.szTip = APP_NAME,
		.uVersion = NOTIFYICON_VERSION_4
	},
	m_whiteIconResource(whiteIconResource),
	m_darkIconResource(darkIconResource),
	m_ShowPreference(false),
	m_CurrentlyShowing(false),
	m_TaskbarCreatedMessage(Window::RegisterMessage(WM_TASKBARCREATED)),
	m_Ssudm(ssudm)
{
#ifdef SIGNED_BUILD
	m_IconData.uFlags |= NIF_GUID;
	m_IconData.guidItem = iconId;
#else
	// good enough method to get an unique id
	m_IconData.uID = iconId.Data1;
#endif

	LoadThemedIcon();

#ifdef SIGNED_BUILD
	// Clear icon from a previous instance that didn't cleanly exit.
	// Errors if instance cleanly exited, so avoid logging it.
	// Only works when using GUIDs.
	Shell_NotifyIcon(NIM_DELETE, &m_IconData);
#endif
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

void TrayIcon::SendNotification(uint16_t textResource, DWORD infoFlags)
{
	if (m_CurrentlyShowing)
	{
		// copy the data because if explorer restarts or the theme/settings change we don't want to re-send the notification
		auto data = m_IconData;
		data.uFlags |= NIF_INFO;
		data.dwInfoFlags = infoFlags;
		// don't set szInfoTitle, the OS will show the app name already.

		Localization::LoadLocalizedResourceString(textResource, hinstance()).copy(data.szInfo, std::size(data.szInfo));

		Notify(NIM_MODIFY, &data);
	}
}

TrayIcon::~TrayIcon() noexcept(false)
{
	Hide();
}
