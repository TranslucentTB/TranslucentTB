#pragma once
#include "contextmenu.hpp"
#include "trayicon.hpp"
#include <windowsx.h>

#include "../dynamicloader.hpp"
#include "../uwp/uwp.hpp"

template<typename T>
class TrayContextMenu : public TrayIcon, public ContextMenu<T> {
private:
	static constexpr POINT PointFromParam(WPARAM wParam) noexcept
	{
		return {
			.x = GET_X_LPARAM(wParam),
			.y = GET_Y_LPARAM(wParam)
		};
	}

protected:
	template<typename... Args>
	inline TrayContextMenu(const GUID &iconId, const wchar_t *whiteIconResource, const wchar_t *darkIconResource, DynamicLoader &loader, Args&&... args) :
		TrayIcon(iconId, whiteIconResource, darkIconResource, loader.ShouldSystemUseDarkMode()),
		ContextMenu<T>(std::forward<Args>(args)...)
	{
		if (const auto admfw = loader.AllowDarkModeForWindow())
		{
			admfw(m_WindowHandle, true);
		}
	}

	inline LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam) override
	{
		switch (uMsg)
		{
		case WM_SETTINGCHANGE:
		case WM_THEMECHANGED:
			if (const auto coreWin = UWP::GetCoreWindow())
			{
				// forward theme changes to the fake core window
				// so that they propagate to our islands
				coreWin.send_message(uMsg, wParam, lParam);
			}

			break;

		default:
			if (uMsg == TRAY_CALLBACK)
			{
				switch (LOWORD(lParam))
				{
				case WM_CONTEXTMENU:
				case WM_LBUTTONUP:
				case NIN_KEYSELECT:
				case NIN_SELECT:
					if (!SetForegroundWindow(m_WindowHandle))
					{
						MessagePrint(spdlog::level::info, L"Failed to set window as foreground window.");
					}

					if (const auto rect = GetTrayRect())
					{
						this->RefreshMenu();
						this->ShowAt(*rect, PointFromParam(wParam));
						post_message(WM_NULL);
					}

					return 0;

				case NIN_POPUPOPEN:
					if (const auto rect = GetTrayRect())
					{
						this->ShowTooltip(*rect);
					}
					return 0;

				case NIN_POPUPCLOSE:
					this->HideTooltip();
					return 0;
				}
			}
				
		}

		return TrayIcon::MessageHandler(uMsg, wParam, lParam);
	}
};
