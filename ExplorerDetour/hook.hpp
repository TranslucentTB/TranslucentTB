#pragma once
#include"../TranslucentTB/arch.h"
#include <mutex>
#include <tuple>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <windef.h>

#include "../TranslucentTB/swcadata.hpp"

#ifdef _EXPLORERDETOUR_DLL
#define EXPLORERHOOK_EXPORT dllexport
#else
#define EXPLORERHOOK_EXPORT dllimport
#endif

class Hook {
private:
	using pSWCA = std::add_pointer_t<BOOL WINAPI(HWND, swca::WINCOMPATTRDATA *)>;
	static pSWCA SetWindowCompositionAttribute;

	static std::mutex m_taskbarsLock;
	static std::unordered_set<HWND> m_taskbars;

	static std::mutex m_excludedTaskbarsLock;
	static std::unordered_set<HWND> m_excludedTaskbars;

	static std::mutex m_initDoneLock;
	static bool m_initDone;

	static unsigned int m_PushWindowMsg;
	static unsigned int m_ExcludeWindowMsg;
	static unsigned int m_IncludeWindowMsg;

	static BOOL WINAPI SetWindowCompositionAttributeDetour(HWND hWnd, swca::WINCOMPATTRDATA *data);
	static LRESULT CALLBACK CallWndProc(int nCode, WPARAM wParam, LPARAM lParam);

	friend BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID);

public:
	__declspec(EXPLORERHOOK_EXPORT) static std::pair<HHOOK, HRESULT> HookExplorer(HWND taskbar);
	__declspec(EXPLORERHOOK_EXPORT) static void ExcludeTaskbar(HWND taskbar);
	__declspec(EXPLORERHOOK_EXPORT) static void IncludeTaskbar(HWND taskbar);

	__declspec(EXPLORERHOOK_EXPORT) static std::tuple<uint8_t, uint8_t, uint8_t> GetDetoursVersion();
};

#undef EXPLORERHOOK_EXPORT