#pragma once
#include <wil/resource.h>

#include "tray/trayicon.hpp"
#include "windows/messagewindow.hpp"
#include "undoc/uxtheme.hpp"

class DarkThemeManager {
private:
	static const wil::unique_hmodule uxtheme;
	static const PFN_SET_PREFERRED_APP_MODE SetPreferredAppMode;
	static const PFN_ALLOW_DARK_MODE_FOR_WINDOW AllowDarkModeForWindow;
	static const PFN_SHOULD_SYSTEM_USE_DARK_MODE ShouldSystemUseDarkMode;

	inline static bool IsDarkModeAvailable()
	{
		return SetPreferredAppMode && AllowDarkModeForWindow && ShouldSystemUseDarkMode;
	}

public:
	static bool AllowDarkModeForApp();
	static void EnableDarkModeForWindow(MessageWindow &window);
	static void EnableDarkModeForTrayIcon(TrayIcon &icon, const wchar_t *darkIcon, const wchar_t *brightIcon);
};