#pragma once
#include "arch.h"
#include <guiddef.h>
#include <windef.h>

#ifdef EXPLORERHOOKS_EXPORTS
#define EXPLORERHOOKS_API __declspec(dllexport)
#else
#define EXPLORERHOOKS_API __declspec(dllimport)
#endif

class ExplorerHooks {
private:
	static constexpr GUID EXPLORER_PAYLOAD = { 0xF5E0B1A9, 0x9D5D, 0x4EB3, { 0x8F, 0x6D, 0x1F, 0x4D, 0x43, 0xD7, 0xCD, 0x0E } };

	static LRESULT CALLBACK CallWndProc(int nCode, WPARAM wParam, LPARAM lParam) noexcept;

	static bool IsInExplorer() noexcept;

#ifdef EXPLORERHOOKS_EXPORTS
	friend BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID) noexcept;
#endif

public:
	EXPLORERHOOKS_API static HHOOK Inject(HWND window) noexcept;
};
