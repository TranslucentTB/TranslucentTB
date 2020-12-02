#pragma once
#include "arch.h"
#include <libloaderapi.h>
#include <windef.h>
#include <winuser.h>
#include <wil/resource.h>

#include "undoc/user32.hpp"
#include "undoc/uxtheme.hpp"
#include "../ProgramLog/error/win32.hpp"

class DynamicLoader {
	wil::unique_hmodule m_User32, m_UxTheme;

	PFN_SHOULD_APPS_USE_DARK_MODE m_Saudm = nullptr;

public:
	inline DynamicLoader()
	{
		m_User32.reset(LoadLibraryEx(L"user32.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32));
		if (!m_User32)
		{
			LastErrorHandle(spdlog::level::critical, L"Failed to load user32.dll");
		}

		m_UxTheme.reset(LoadLibraryEx(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32));
		if (m_UxTheme)
		{
			m_Saudm = reinterpret_cast<PFN_SHOULD_APPS_USE_DARK_MODE>(GetProcAddress(m_UxTheme.get(), MAKEINTRESOURCEA(132)));
			if (!m_Saudm)
			{
				LastErrorHandle(spdlog::level::warn, L"Failed to get address of ShouldAppsUseDarkMode");
			}
		}
		else
		{
			LastErrorHandle(spdlog::level::warn, L"Failed to load uxtheme.dll");
		}
	}

	DynamicLoader(const DynamicLoader &) = delete;
	DynamicLoader &operator =(const DynamicLoader &) = delete;

	// Stored by TaskbarAttributeWorker, but only 1 instance
	inline PFN_SET_WINDOW_COMPOSITION_ATTRIBUTE SetWindowCompositionAttribute() const
	{
		const auto fn = reinterpret_cast<PFN_SET_WINDOW_COMPOSITION_ATTRIBUTE>(GetProcAddress(m_User32.get(), "SetWindowCompositionAttribute"));
		if (!fn)
		{
			LastErrorHandle(spdlog::level::critical, L"Failed to get address of SetWindowCompositionAttribute");
		}

		return fn;
	}

	// Used in Application constructor
	inline PFN_SET_PREFERRED_APP_MODE SetPreferredAppMode() const
	{
		if (m_UxTheme)
		{
			const auto fn = reinterpret_cast<PFN_SET_PREFERRED_APP_MODE>(GetProcAddress(m_UxTheme.get(), MAKEINTRESOURCEA(135)));
			if (!fn)
			{
				LastErrorHandle(spdlog::level::warn, L"Failed to get address of SetPreferredAppMode");
			}

			return fn;
		}
		else
		{
			return nullptr;
		}
	}

	// Used in TrayContextMenu constructor, but only 1 TrayContextMenu instance
	inline PFN_ALLOW_DARK_MODE_FOR_WINDOW AllowDarkModeForWindow() const
	{
		if (m_UxTheme)
		{
			const auto fn = reinterpret_cast<PFN_ALLOW_DARK_MODE_FOR_WINDOW>(GetProcAddress(m_UxTheme.get(), MAKEINTRESOURCEA(133)));
			if (!fn)
			{
				LastErrorHandle(spdlog::level::warn, L"Failed to get address of AllowDarkModeForWindow");
			}

			return fn;
		}
		else
		{
			return nullptr;
		}
	}

	// Stored in TrayIcon, but only 1 TrayIcon instance
	inline PFN_SHOULD_SYSTEM_USE_DARK_MODE ShouldSystemUseDarkMode() const
	{
		if (m_UxTheme)
		{
			const auto fn = reinterpret_cast<PFN_SHOULD_SYSTEM_USE_DARK_MODE>(GetProcAddress(m_UxTheme.get(), MAKEINTRESOURCEA(138)));
			if (!fn)
			{
				LastErrorHandle(spdlog::level::warn, L"Failed to get address of ShouldSystemUseDarkMode");
			}

			return fn;
		}
		else
		{
			return nullptr;
		}
	}

	// Stored in each XamlPageHost
	constexpr PFN_SHOULD_APPS_USE_DARK_MODE ShouldAppsUseDarkMode() const noexcept
	{
		return m_Saudm;
	}
};
