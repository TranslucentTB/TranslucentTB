#include "explorerhooks.hpp"
#include <WinBase.h>
#include <detours/detours.h>
#include <wil/result.h>
#include <WinUser.h>

LRESULT CALLBACK ExplorerHooks::CallWndProc(int nCode, WPARAM wParam, LPARAM lParam) noexcept
{
	// Placeholder
	return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

void *ExplorerHooks::FindExplorerPayload() noexcept
{
	return DetourFindPayloadEx(EXPLORER_PAYLOAD, nullptr);
}

void ExplorerHooks::FreeExplorerPayload(void *payload) noexcept
{
	FAIL_FAST_IF_WIN32_BOOL_FALSE(DetourFreePayload(payload));
}
