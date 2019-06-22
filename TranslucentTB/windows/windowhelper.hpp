#pragma once
#include <wil/common.h>

#include "window.hpp"

class WindowHelper {
private:
	inline static Window GetLastVisibleActivePopup(Window window)
	{
		Window lastPopup = GetLastActivePopup(window);
		if (IsVisibleWindow(lastPopup))
		{
			return lastPopup;
		}
		else if (lastPopup == window)
		{
			return Window::NullWindow;
		}
		else
		{
			return GetLastVisibleActivePopup(lastPopup);
		}
	}

public:
	// Checks if the window is valid, visible, not cloaked and on the current virtual desktop.
	inline static bool IsVisibleWindow(Window window)
	{
		return window.valid() && window.visible() && !window.cloaked() && window.on_current_desktop();
	}

	// Used to determine if the window is a "user" window (shows up in Alt-Tab)
	inline static bool IsUserWindow(Window window)
	{
		const auto exstyle = window.extended_style();

		return
			IsVisibleWindow(window) &&
			!WI_IsFlagSet(window.titlebar_info().rgstate[0], STATE_SYSTEM_INVISIBLE) && // TODO: for some reason when dragging windbg, it has no titlebar
			!WI_IsAnyFlagSet(exstyle, WS_EX_TOOLWINDOW | WS_EX_MDICHILD) &&
			(WI_IsFlagSet(exstyle, WS_EX_APPWINDOW) || GetLastVisibleActivePopup(window) == window);
	}
};