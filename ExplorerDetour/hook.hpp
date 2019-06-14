#pragma once
#include "arch.h"
#include <atomic>
#include <tuple>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <windef.h>

#include "undoc/swca.hpp"
#include "window.hpp"

#ifdef _EXPLORERDETOUR_DLL
#define EXPLORERHOOK_EXPORT dllexport
#else
#define EXPLORERHOOK_EXPORT dllimport
#endif

class Hook {
private:
	static PFN_SET_WINDOW_COMPOSITION_ATTRIBUTE SetWindowCompositionAttribute;
	static std::atomic_flag s_IsHooked;

	static BOOL WINAPI SetWindowCompositionAttributeDetour(HWND hWnd, const WINDOWCOMPOSITIONATTRIBDATA *data);
	static LRESULT CALLBACK CallWndProc(int nCode, WPARAM wParam, LPARAM lParam) noexcept;

#ifdef _EXPLORERDETOUR_DLL
	friend BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID) noexcept;
#endif

public:
	__declspec(EXPLORERHOOK_EXPORT) static std::pair<HHOOK, HRESULT> HookExplorer(Window taskbar);
	__declspec(EXPLORERHOOK_EXPORT) static std::tuple<uint8_t, uint8_t, uint8_t> GetDetoursVersion();
};

#undef EXPLORERHOOK_EXPORT