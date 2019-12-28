#pragma once
#include "arch.h"
#include <libloaderapi.h>
#include <windef.h>
#include <winuser.h>
#include <wil/resource.h>

#include "user32.hpp"

#ifdef _TRANSLUCENTTB_EXE
#include "../../ProgramLog/error/win32.hpp"
#include "uxtheme.hpp"
#endif

namespace DynamicLoader {
	inline HMODULE user32()
	{
		static wil::unique_hmodule lib;
		if (!lib)
		{
			lib.reset(LoadLibraryEx(L"user32.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32));
#ifdef _TRANSLUCENTTB_EXE
			if (!lib)
			{
				LastErrorHandle(spdlog::level::critical, L"Failed to load user32.dll");
			}
#endif
		}

		return lib.get();
	}

	// Both users store the value themselves:
	// - ExplorerDetour for Detours to give a trampoline
	// - TranslucentTB for reduced overhead (in hotpath)
	inline PFN_SET_WINDOW_COMPOSITION_ATTRIBUTE SetWindowCompositionAttribute()
	{
		// Exported by name
		PFN_SET_WINDOW_COMPOSITION_ATTRIBUTE fn = reinterpret_cast<PFN_SET_WINDOW_COMPOSITION_ATTRIBUTE>(GetProcAddress(user32(), "SetWindowCompositionAttribute"));
#ifdef _TRANSLUCENTTB_EXE
		if (!fn)
		{
			LastErrorHandle(spdlog::level::critical, L"Failed to get address of SetWindowCompositionAttribute");
		}
#endif

		return fn;
	}

#ifdef _TRANSLUCENTTB_EXE
	inline HMODULE uxtheme()
	{
		static wil::unique_hmodule lib;
		if (!lib)
		{
			lib.reset(LoadLibraryEx(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32));
			if (!lib)
			{
				LastErrorHandle(spdlog::level::warn, L"Failed to load uxtheme.dll");
			}
		}

		return lib.get();
	}

	// Used only once
	inline PFN_SET_PREFERRED_APP_MODE SetPreferredAppMode()
	{
		PFN_SET_PREFERRED_APP_MODE fn = reinterpret_cast<PFN_SET_PREFERRED_APP_MODE>(GetProcAddress(uxtheme(), MAKEINTRESOURCEA(135)));
		if (!fn)
		{
			LastErrorHandle(spdlog::level::warn, L"Failed to get address of SetPreferredAppMode");
		}

		return fn;
	}

	// Used each new MessageWindow instance
	inline PFN_ALLOW_DARK_MODE_FOR_WINDOW AllowDarkModeForWindow()
	{
		static PFN_ALLOW_DARK_MODE_FOR_WINDOW fn;
		if (!fn)
		{
			fn = reinterpret_cast<PFN_ALLOW_DARK_MODE_FOR_WINDOW>(GetProcAddress(uxtheme(), MAKEINTRESOURCEA(133)));
			if (!fn)
			{
				LastErrorHandle(spdlog::level::warn, L"Failed to get address of AllowDarkModeForWindow");
			}
		}

		return fn;
	}

	// Used each time the system theme/settings change
	inline PFN_SHOULD_SYSTEM_USE_DARK_MODE ShouldSystemUseDarkMode()
	{
		static PFN_SHOULD_SYSTEM_USE_DARK_MODE fn;
		if (!fn)
		{
			fn = reinterpret_cast<PFN_SHOULD_SYSTEM_USE_DARK_MODE>(GetProcAddress(uxtheme(), MAKEINTRESOURCEA(138)));
			if (!fn)
			{
				LastErrorHandle(spdlog::level::warn, L"Failed to get address of ShouldSystemUseDarkMode");
			}
		}

		return fn;
	}
#endif
}
