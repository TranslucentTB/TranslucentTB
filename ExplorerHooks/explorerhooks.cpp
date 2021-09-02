#include "explorerhooks.hpp"
#include <cstdint>
#include <WinBase.h>
#include <detours/detours.h>
#include <processthreadsapi.h>
#include <wil/resource.h>
#include <wil/result.h>
#include <wil/win32_helpers.h>
#include <WinUser.h>

HHOOK InjectExplorerHook(HWND window) noexcept
{
	DWORD pid = 0;
	const DWORD tid = GetWindowThreadProcessId(window, &pid);

	wil::unique_process_handle proc(OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, pid));
	if (!proc) [[unlikely]]
	{
		return nullptr;
	}

	if (!DetourFindRemotePayload(proc.get(), ExplorerHooks::EXPLORER_PAYLOAD, nullptr))
	{
		proc.reset(OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_WRITE, false, pid));
		if (!proc) [[unlikely]]
		{
			return nullptr;
		}

		static constexpr uint32_t content = 0xDEADBEEF;
		if (!DetourCopyPayloadToProcess(proc.get(), ExplorerHooks::EXPLORER_PAYLOAD, &content, sizeof(content))) [[unlikely]]
		{
			return nullptr;
		}
	}

	return SetWindowsHookEx(WH_CALLWNDPROC, ExplorerHooks::CallWndProc, wil::GetModuleInstanceHandle(), tid);
}

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
