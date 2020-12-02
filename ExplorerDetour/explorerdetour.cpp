#include "explorerdetour.hpp"
#include <cstdint>
#include <detours/detours.h>
#include <wil/win32_helpers.h>
#include <WinUser.h>

#include "constants.hpp"
#include "detourtransaction.hpp"
#include "util/string_macros.hpp"

wil::unique_hmodule ExplorerDetour::s_User32;
PFN_SET_WINDOW_COMPOSITION_ATTRIBUTE ExplorerDetour::SetWindowCompositionAttribute;
UINT ExplorerDetour::s_RequestAttribute;
wil::unique_hheap ExplorerDetour::s_Heap;
bool ExplorerDetour::s_DetourInstalled;

BOOL WINAPI ExplorerDetour::SetWindowCompositionAttributeDetour(HWND hWnd, const WINDOWCOMPOSITIONATTRIBDATA *data) noexcept
{
	if (data->Attrib == WCA_ACCENT_POLICY)
	{
		if (const auto worker = FindWindow(WORKER_WINDOW.c_str(), WORKER_WINDOW.c_str()))
		{
			// avoid freezing Explorer if our main process is frozen
			DWORD_PTR result = 0;
			if (SendMessageTimeout(worker, s_RequestAttribute, 0, reinterpret_cast<LPARAM>(hWnd), SMTO_ABORTIFHUNG | SMTO_BLOCK | SMTO_ERRORONEXIT, 50, &result) && result)
			{
				return true;
			}
		}
	}

	return SetWindowCompositionAttribute(hWnd, data);
}

LRESULT CALLBACK ExplorerDetour::CallWndProc(int nCode, WPARAM wParam, LPARAM lParam) noexcept
{
	// Placeholder
	return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

bool ExplorerDetour::IsInExplorer() noexcept
{
	return DetourFindPayloadEx(EXPLORERDETOUR_PAYLOAD, nullptr);
}

bool ExplorerDetour::Install() noexcept
{
	if (!s_User32)
	{
		s_User32.reset(LoadLibraryEx(L"user32.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32));
		if (!s_User32)
		{
			return false;
		}
	}

	if (!SetWindowCompositionAttribute)
	{
		SetWindowCompositionAttribute = reinterpret_cast<PFN_SET_WINDOW_COMPOSITION_ATTRIBUTE>(GetProcAddress(s_User32.get(), UTIL_STRINGIFY_UTF8(SetWindowCompositionAttribute)));
		if (!SetWindowCompositionAttribute)
		{
			return false;
		}
	}

	if (!s_RequestAttribute)
	{
		s_RequestAttribute = RegisterWindowMessage(WM_TTBHOOKREQUESTREFRESH.c_str());
		if (!s_RequestAttribute)
		{
			return false;
		}
	}

	if (!s_Heap)
	{
		s_Heap.reset(HeapCreate(HEAP_NO_SERIALIZE, 0, 0));
		if (!s_Heap)
		{
			return false;
		}
	}

	if (!s_DetourInstalled)
	{
		DetourTransaction transaction;
		if (transaction.begin() &&
			transaction.update_all_threads() &&
			transaction.attach(SetWindowCompositionAttribute, SetWindowCompositionAttributeDetour) &&
			transaction.commit())
		{
			s_DetourInstalled = true;
		}
		else
		{
			return false;
		}
	}

	return true;
}

bool ExplorerDetour::Uninstall() noexcept
{
	if (s_DetourInstalled)
	{
		DetourTransaction transaction;
		if (transaction.begin() &&
			transaction.update_all_threads() &&
			transaction.detach(SetWindowCompositionAttribute, SetWindowCompositionAttributeDetour) &&
			transaction.commit())
		{
			s_DetourInstalled = false;
		}
		else
		{
			return false;
		}
	}

	return true;
}

wil::unique_hhook ExplorerDetour::Inject(HWND window) noexcept
{
	DWORD pid;
	const DWORD tid = GetWindowThreadProcessId(window, &pid);

	wil::unique_process_handle proc(OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, pid));
	if (!proc)
	{
		return nullptr;
	}

	if (!DetourFindRemotePayload(proc.get(), EXPLORERDETOUR_PAYLOAD, nullptr))
	{
		proc.reset(OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_WRITE, false, pid));
		if (!proc)
		{
			return nullptr;
		}

		static constexpr uint32_t content = 0xDEADBEEF;
		if (!DetourCopyPayloadToProcess(proc.get(), EXPLORERDETOUR_PAYLOAD, &content, sizeof(content)))
		{
			return nullptr;
		}
	}

	return wil::unique_hhook(SetWindowsHookEx(WH_CALLWNDPROC, CallWndProc, wil::GetModuleInstanceHandle(), tid));
}
