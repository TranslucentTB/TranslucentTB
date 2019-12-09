#pragma once
#include "trayicon.hpp"
#include "contextmenu.hpp"

class TrayContextMenu : public TrayIcon, public ContextMenu {
public:
	inline TrayContextMenu(const GUID &iconId, Util::null_terminated_wstring_view className,
		Util::null_terminated_wstring_view windowName, const wchar_t *whiteIconResource, const wchar_t *darkIconResource,
		const wchar_t *menuResource, HINSTANCE hInstance) :
		TrayIcon(iconId, className, windowName, whiteIconResource, darkIconResource, hInstance),
		ContextMenu(menuResource, hInstance)
	{ }

protected:
	inline LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam) override
	{
		switch (uMsg)
		{
		case WM_INITMENU:
			RefreshMenu();
			return 0;

		default:
			if (uMsg == TRAY_CALLBACK)
			{
				if (LOWORD(lParam) == WM_CONTEXTMENU || LOWORD(lParam) == WM_LBUTTONDOWN)
				{
					if (!SetForegroundWindow(m_WindowHandle))
					{
						MessagePrint(spdlog::level::info, L"Failed to set window as foreground window.");
					}

					ShowAtCursor(m_WindowHandle);
				}

				return 0;
			}
			else
			{
				return TrayIcon::MessageHandler(uMsg, wParam, lParam);
			}
		}
	}
};
