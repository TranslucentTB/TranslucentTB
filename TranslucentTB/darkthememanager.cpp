#include "darkthememanager.hpp"

const wil::unique_hmodule DarkThemeManager::uxtheme(LoadLibraryEx(UXTHEME_DLL, nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32));

const PFN_SET_PREFERRED_APP_MODE DarkThemeManager::SetPreferredAppMode =
	reinterpret_cast<PFN_SET_PREFERRED_APP_MODE>(GetProcAddress(uxtheme.get(), SPAM_ORDINAL));

const PFN_ALLOW_DARK_MODE_FOR_WINDOW DarkThemeManager::AllowDarkModeForWindow =
	reinterpret_cast<PFN_ALLOW_DARK_MODE_FOR_WINDOW>(GetProcAddress(uxtheme.get(), ADMFW_ORDINAL));

const PFN_SHOULD_SYSTEM_USE_DARK_MODE DarkThemeManager::ShouldSystemUseDarkMode =
	reinterpret_cast<PFN_SHOULD_SYSTEM_USE_DARK_MODE>(GetProcAddress(uxtheme.get(), SSUDM_ORDINAL));

bool DarkThemeManager::AllowDarkModeForApp()
{
	if (IsDarkModeAvailable())
	{
		SetPreferredAppMode(PreferredAppMode::AllowDark);
		return true;
	}
	else
	{
		return false;
	}
}

void DarkThemeManager::EnableDarkModeForWindow(Window window)
{
	if (IsDarkModeAvailable())
	{
		AllowDarkModeForWindow(window, true);
	}
}

void DarkThemeManager::EnableDarkModeForTrayIcon(TrayIcon &icon, const wchar_t *darkIcon, const wchar_t *brightIcon)
{
	if (IsDarkModeAvailable())
	{
		const auto callback = [&icon, darkIcon, brightIcon](...)
		{
			icon.SetIcon(ShouldSystemUseDarkMode() ? darkIcon : brightIcon);
			return 0;
		};

		callback();
		icon.GetWindow().RegisterCallback(WM_SETTINGCHANGE, callback);
	}
}
