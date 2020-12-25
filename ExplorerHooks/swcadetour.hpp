#pragma once
#include "arch.h"
#include <windef.h>

#include "undoc/user32.hpp"

class SWCADetour {
private:
	static HMODULE s_User32;
	static PFN_SET_WINDOW_COMPOSITION_ATTRIBUTE SetWindowCompositionAttribute;
	static UINT s_RequestAttribute;
	static HANDLE s_Heap;
	static bool s_DetourInstalled;

	static BOOL WINAPI FunctionDetour(HWND hWnd, const WINDOWCOMPOSITIONATTRIBDATA *data) noexcept;

	static void Install() noexcept;
	static void Uninstall() noexcept;

#ifdef EXPLORERHOOKS_EXPORTS
	friend class DetourTransaction;
	friend BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID) noexcept;
#endif
};
