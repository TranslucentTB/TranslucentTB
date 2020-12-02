#pragma once
#include "arch.h"
#include <guiddef.h>
#include <heapapi.h>
#include <libloaderapi.h>
#include <wil/resource.h>

#include "undoc/user32.hpp"

#ifdef EXPLORERDETOUR_EXPORTS
#define EXPLORERDETOUR_API __declspec(dllexport)
#else
#define EXPLORERDETOUR_API __declspec(dllimport)
#endif

class ExplorerDetour {
private:
	static constexpr GUID EXPLORERDETOUR_PAYLOAD = { 0xF5E0B1A9, 0x9D5D, 0x4EB3, { 0x8F, 0x6D, 0x1F, 0x4D, 0x43, 0xD7, 0xCD, 0x0E } };

	static wil::unique_hmodule s_User32;
	static PFN_SET_WINDOW_COMPOSITION_ATTRIBUTE SetWindowCompositionAttribute;
	static UINT s_RequestAttribute;
	static wil::unique_hheap s_Heap;
	static bool s_DetourInstalled;

	static BOOL WINAPI SetWindowCompositionAttributeDetour(HWND hWnd, const WINDOWCOMPOSITIONATTRIBDATA *data) noexcept;
	static LRESULT CALLBACK CallWndProc(int nCode, WPARAM wParam, LPARAM lParam) noexcept;

	static bool IsInExplorer() noexcept;
	static bool Install() noexcept;
	static bool Uninstall() noexcept;

#ifdef EXPLORERDETOUR_EXPORTS
	friend class DetourTransaction;
	friend BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID) noexcept;
#endif

public:
	EXPLORERDETOUR_API static wil::unique_hhook Inject(HWND window) noexcept;
};
