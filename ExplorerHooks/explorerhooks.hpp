#pragma once
#include "arch.h"
#include <guiddef.h>
#include <windef.h>

class ExplorerHooks {
private:
	static constexpr GUID EXPLORER_PAYLOAD = { 0xF5E0B1A9, 0x9D5D, 0x4EB3, { 0x8F, 0x6D, 0x1F, 0x4D, 0x43, 0xD7, 0xCD, 0x0E } };

	static LRESULT CALLBACK CallWndProc(int nCode, WPARAM wParam, LPARAM lParam) noexcept;

	static void *FindExplorerPayload() noexcept;
	static void FreeExplorerPayload(void *payload) noexcept;

public:
#ifdef EXPLORERHOOKS_EXPORTS
	friend BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID) noexcept;

	__declspec(dllexport)
#else
	__declspec(dllimport)
#endif
	static HHOOK Inject(HWND window) noexcept;
};
