#include "api.hpp"
#include <cstdint>
#include <WinBase.h>
#include <detours/detours.h>
#include <processthreadsapi.h>
#include <wil/resource.h>
#include <wil/win32_helpers.h>
#include <WinUser.h>

#include "constants.hpp"

LRESULT CALLBACK CallWndProc(int nCode, WPARAM wParam, LPARAM lParam) noexcept
{
	// Placeholder
	return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

HHOOK InjectExplorerHook(HWND window) noexcept
{
	DWORD pid = 0;
	const DWORD tid = GetWindowThreadProcessId(window, &pid);

	wil::unique_process_handle proc(OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, pid));
	if (!proc) [[unlikely]]
	{
		return nullptr;
	}

	if (!DetourFindRemotePayload(proc.get(), EXPLORER_PAYLOAD, nullptr))
	{
		proc.reset(OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_WRITE, false, pid));
		if (!proc) [[unlikely]]
		{
			return nullptr;
		}

		static constexpr uint32_t content = 0xDEADBEEF;
		if (!DetourCopyPayloadToProcess(proc.get(), EXPLORER_PAYLOAD, &content, sizeof(content))) [[unlikely]]
		{
			return nullptr;
		}
	}

	return SetWindowsHookEx(WH_CALLWNDPROC, CallWndProc, wil::GetModuleInstanceHandle(), tid);
}
