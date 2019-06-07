#include "hook.hpp"
#include <WinBase.h>
#include <detours.h>
#include <winerror.h>
#include <WinUser.h>

#include "constants.hpp"
#include "dlldata.hpp"
#include "detourexception.hpp"
#include "detourtransaction.hpp"

PFN_SET_WINDOW_COMPOSITION_ATTRIBUTE Hook::SetWindowCompositionAttribute =
	reinterpret_cast<PFN_SET_WINDOW_COMPOSITION_ATTRIBUTE>(GetProcAddress(GetModuleHandle(L"user32.dll"), "SetWindowCompositionAttribute"));

std::mutex Hook::s_initLock;
Hook::InitializationState Hook::s_initState = Hook::InitializationState::NotInitialized;

const Window Hook::s_TTBMsgWnd = Window::Find(WORKER_WINDOW, WORKER_WINDOW);

BOOL Hook::SetWindowCompositionAttributeDetour(HWND hWnd, const WINDOWCOMPOSITIONATTRIBDATA *data)
{
	if (data->Attrib == WCA_ACCENT_POLICY && s_TTBMsgWnd.send_message(WM_TTBHOOKREQUESTREFRESH, 0, reinterpret_cast<LPARAM>(hWnd)))
	{
		return TRUE;
	}

	return SetWindowCompositionAttribute(hWnd, data);
}

LRESULT Hook::CallWndProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION)
	{
		std::lock_guard guard(s_initLock);
		if (s_initState == InitializationState::NotInitialized)
		{
			try
			{
				DetourTransaction transaction;

				transaction.update_current_thread();
				transaction.attach(Hook::SetWindowCompositionAttribute, Hook::SetWindowCompositionAttributeDetour);
				transaction.commit();

				s_initState = InitializationState::InitializationDone;
			}
			catch (const DetourException &err)
			{
				MessageBox(
					Window::NullWindow,
					(L"Failed to install detour: " + err.message()).c_str(),
					NAME L" Hook - Error",
					MB_ICONERROR | MB_OK | MB_SETFOREGROUND
				);
				s_initState = InitializationState::InitializationFailed;
			}
		}
	}

	return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

std::pair<HHOOK, HRESULT> Hook::HookExplorer(HWND taskbar)
{
	HHOOK hook = SetWindowsHookEx(WH_CALLWNDPROC, CallWndProc, DllData::GetInstanceHandle(), GetWindowThreadProcessId(taskbar, nullptr));
	if (!hook)
	{
		return { nullptr, HRESULT_FROM_WIN32(GetLastError()) };
	}

	return { hook, S_OK };
}

std::tuple<uint8_t, uint8_t, uint8_t> Hook::GetDetoursVersion()
{
	const uint32_t version = DETOURS_VERSION;
	return { (version & 0xF0000) >> 16, (version & 0xF00) >> 8, version & 0xF };
}
