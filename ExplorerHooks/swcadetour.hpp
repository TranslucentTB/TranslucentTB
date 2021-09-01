#pragma once
#include "arch.h"
#include <wil/resource.h>
#include <windef.h>

#include "undoc/user32.hpp"

class SWCADetour {
private:
	static void FreeLibraryFailFast(HMODULE hModule) noexcept;
	using unique_module_failfast = wil::unique_any<HMODULE, decltype(&FreeLibraryFailFast), FreeLibraryFailFast>;

	static unique_module_failfast s_User32;
	static PFN_SET_WINDOW_COMPOSITION_ATTRIBUTE SetWindowCompositionAttribute;
	static UINT s_RequestAttribute;
	static bool s_DetourInstalled;

	static BOOL WINAPI FunctionDetour(HWND hWnd, const WINDOWCOMPOSITIONATTRIBDATA *data) noexcept;

	static void Install() noexcept;
	static void Uninstall() noexcept;

	friend BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID) noexcept;
};
