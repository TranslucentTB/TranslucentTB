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
	inline constexpr bool is_exception_free_v =
#ifdef _TRANSLUCENTTB_EXE
		false;
#else
		true;
#endif

	inline HMODULE user32() noexcept(is_exception_free_v)
	{
		static const wil::unique_hmodule lib = []
		{
			wil::unique_hmodule library(LoadLibraryEx(L"user32.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32));
#ifdef _TRANSLUCENTTB_EXE
			if (!library)
			{
				LastErrorHandle(spdlog::level::critical, L"Failed to load user32.dll");
			}
#endif
			return library;
		}();

		return lib.get();
	}

	// Both users store the value themselves:
	// - ExplorerDetour for Detours to give a trampoline
	// - TranslucentTB for reduced overhead (in hotpath)
	inline PFN_SET_WINDOW_COMPOSITION_ATTRIBUTE SetWindowCompositionAttribute() noexcept(is_exception_free_v)
	{
		if (const auto hUser32 = user32())
		{
			const auto fn = reinterpret_cast<PFN_SET_WINDOW_COMPOSITION_ATTRIBUTE>(GetProcAddress(hUser32, "SetWindowCompositionAttribute"));
#ifdef _TRANSLUCENTTB_EXE
			if (!fn)
			{
				LastErrorHandle(spdlog::level::critical, L"Failed to get address of SetWindowCompositionAttribute");
			}
#endif

			return fn;
		}
		else
		{
			return nullptr;
		}
	}

#ifdef _TRANSLUCENTTB_EXE
	inline HMODULE uxtheme()
	{
		static const wil::unique_hmodule lib = []
		{
			wil::unique_hmodule library(LoadLibraryEx(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32));
			if (!library)
			{
				LastErrorHandle(spdlog::level::warn, L"Failed to load uxtheme.dll");
			}

			return library;
		}();

		return lib.get();
	}

	// Used only once
	inline PFN_SET_PREFERRED_APP_MODE SetPreferredAppMode()
	{
		if (const auto hUxtheme = uxtheme())
		{
			const auto fn = reinterpret_cast<PFN_SET_PREFERRED_APP_MODE>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(135)));
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

	// Used each new MessageWindow instance
	inline PFN_ALLOW_DARK_MODE_FOR_WINDOW AllowDarkModeForWindow()
	{
		static const auto fn = []() -> PFN_ALLOW_DARK_MODE_FOR_WINDOW
		{
			if (const auto hUxtheme = uxtheme())
			{
				const auto admfw = reinterpret_cast<PFN_ALLOW_DARK_MODE_FOR_WINDOW>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(133)));
				if (!admfw)
				{
					LastErrorHandle(spdlog::level::warn, L"Failed to get address of AllowDarkModeForWindow");
				}

				return admfw;
			}
			else
			{
				return nullptr;
			}
		}();

		return fn;
	}

	// Used each time the system theme/settings change
	inline PFN_SHOULD_SYSTEM_USE_DARK_MODE ShouldSystemUseDarkMode()
	{
		static const auto fn = []() -> PFN_SHOULD_SYSTEM_USE_DARK_MODE
		{
			if (const auto hUxtheme = uxtheme())
			{
				const auto ssudm = reinterpret_cast<PFN_SHOULD_SYSTEM_USE_DARK_MODE>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(138)));
				if (!ssudm)
				{
					LastErrorHandle(spdlog::level::warn, L"Failed to get address of ShouldSystemUseDarkMode");
				}

				return ssudm;
			}
			else
			{
				return nullptr;
			}
		}();

		return fn;
	}

	// Used each time the system theme/settings change
	inline PFN_SHOULD_APPS_USE_DARK_MODE ShouldAppsUseDarkMode()
	{
		static const auto fn = []() -> PFN_SHOULD_APPS_USE_DARK_MODE
		{
			if (const auto hUxtheme = uxtheme())
			{
				const auto saudm = reinterpret_cast<PFN_SHOULD_APPS_USE_DARK_MODE>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(132)));
				if (!saudm)
				{
					LastErrorHandle(spdlog::level::warn, L"Failed to get address of ShouldAppsUseDarkMode");
				}

				return saudm;
			}
			else
			{
				return nullptr;
			}
		}();

		return fn;
	}
#endif
}
