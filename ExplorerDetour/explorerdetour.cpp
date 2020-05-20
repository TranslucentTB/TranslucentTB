#include "explorerdetour.hpp"
#include <cstdint>
#include <wil/win32_helpers.h>
#include <WinUser.h>

#include "constants.hpp"
#include "detourexception.hpp"
#include "detourtransaction.hpp"
#include "undoc/dynamicloader.hpp"

PFN_SET_WINDOW_COMPOSITION_ATTRIBUTE ExplorerDetour::SetWindowCompositionAttribute;
UINT ExplorerDetour::s_RequestAttribute;
bool ExplorerDetour::s_DetourInstalled;

BOOL WINAPI ExplorerDetour::SetWindowCompositionAttributeDetour(HWND hWnd, const WINDOWCOMPOSITIONATTRIBDATA *data) noexcept
{
	if (data->Attrib == WCA_ACCENT_POLICY)
	{
		if (const auto worker = Window::Find(WORKER_WINDOW, WORKER_WINDOW))
		{
#ifndef _DEBUG
			if (worker.send_message(s_RequestAttribute, 0, reinterpret_cast<LPARAM>(hWnd)))
			{
				return true;
			}
#else
			// avoid freezing Explorer in debug mode
			if (DWORD_PTR result; SendMessageTimeout(worker, s_RequestAttribute, 0, reinterpret_cast<LPARAM>(hWnd), SMTO_ABORTIFHUNG, 50, &result) && result)
			{
				return true;
			}
#endif
		}
	}

	return SetWindowCompositionAttribute(hWnd, data);
}

LRESULT CALLBACK ExplorerDetour::CallWndProc(int nCode, WPARAM wParam, LPARAM lParam) noexcept
{
	// Dummy
	return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

bool ExplorerDetour::IsInExplorer() noexcept
{
	return DetourFindPayloadEx(EXPLORERDETOUR_PAYLOAD, nullptr);
}

bool ExplorerDetour::Install() noexcept
{
	if (!SetWindowCompositionAttribute)
	{
		if (DynamicLoader::user32())
		{
			SetWindowCompositionAttribute = DynamicLoader::SetWindowCompositionAttribute();
			if (!SetWindowCompositionAttribute)
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}

	if (!s_RequestAttribute)
	{
		const auto message = Window::RegisterMessage(WM_TTBHOOKREQUESTREFRESH);
		if (message)
		{
			s_RequestAttribute = *message;
		}
		else
		{
			return false;
		}
	}

	if (!s_DetourInstalled)
	{
		try
		{
			DetourTransaction transaction;

			transaction.update_all_threads();
			transaction.attach(SetWindowCompositionAttribute, SetWindowCompositionAttributeDetour);
			transaction.commit();
		}
		catch (...)
		{
			return false;
		}

		s_DetourInstalled = true;
	}

	return true;
}

void ExplorerDetour::Uninstall()
{
	if (s_DetourInstalled)
	{
		DetourTransaction transaction;

		transaction.update_all_threads();
		transaction.detach(SetWindowCompositionAttribute, SetWindowCompositionAttributeDetour);
		transaction.commit();

		s_DetourInstalled = false;
	}
}

wil::unique_hhook ExplorerDetour::Inject(Window window) noexcept
{
	const auto [tid, pid] = window.thread_process_id();
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
