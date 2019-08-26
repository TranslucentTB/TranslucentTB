#pragma once
#include "arch.h"
#include <guiddef.h>
#include <string>
#include <windef.h>
#include <wil/resource.h>

#include "window.hpp"
#include "undoc/swca.hpp"

#ifdef EXPLORERDETOUR_EXPORTS
#define EXPLORERDETOUR_API __declspec(dllexport)
#else
#define EXPLORERDETOUR_API __declspec(dllimport)
#endif

class ExplorerDetour {
private:
	static constexpr GUID EXPLORERDETOUR_PAYLOAD = { 0xf5e0b1a9, 0x9d5d, 0x4eb3, { 0x8f, 0x6d, 0x1f, 0x4d, 0x43, 0xd7, 0xcd, 0xe } };

	static PFN_SET_WINDOW_COMPOSITION_ATTRIBUTE s_SetWindowCompositionAttribute;
	static std::wstring s_WorkerWindow;
	static UINT s_RequestAttribute;
	static bool s_DetourInstalled;

	static BOOL WINAPI SetWindowCompositionAttributeDetour(HWND hWnd, const WINDOWCOMPOSITIONATTRIBDATA *data) noexcept;
	static LRESULT CALLBACK CallWndProc(int nCode, WPARAM wParam, LPARAM lParam) noexcept;

public:
#ifdef EXPLORERDETOUR_EXPORTS
	static bool IsInExplorer() noexcept;
	static bool Install() noexcept;
	static void Uninstall();
#endif

	EXPLORERDETOUR_API static wil::unique_hhook Inject(Window window) noexcept;
};
