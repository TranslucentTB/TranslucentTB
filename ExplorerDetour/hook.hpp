#pragma once
#include"../TranslucentTB/arch.h"
#include <mutex>
#include <tuple>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <windef.h>

#include "../TranslucentTB/swcadef.h"

#ifdef _EXPLORERDETOUR_DLL
#define EXPLORERHOOK_EXPORT dllexport
#else
#define EXPLORERHOOK_EXPORT dllimport
#endif

class Hook {
private:
	static PFN_SET_WINDOW_COMPOSITION_ATTRIBUTE SetWindowCompositionAttribute;

	static std::mutex m_initDoneLock;
	static bool m_initDone;

	static const HWND m_TTBMsgWnd;
	static BOOL WINAPI SetWindowCompositionAttributeDetour(HWND hWnd, const WINDOWCOMPOSITIONATTRIBDATA *data);
	static LRESULT CALLBACK CallWndProc(int nCode, WPARAM wParam, LPARAM lParam);

	friend BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID);

public:
	__declspec(EXPLORERHOOK_EXPORT) static const unsigned int RequestAttributeRefresh;

	__declspec(EXPLORERHOOK_EXPORT) static std::pair<HHOOK, HRESULT> HookExplorer(HWND taskbar);

	__declspec(EXPLORERHOOK_EXPORT) static std::tuple<uint8_t, uint8_t, uint8_t> GetDetoursVersion();
};

#undef EXPLORERHOOK_EXPORT