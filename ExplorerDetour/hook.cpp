#include "hook.hpp"
#include <WinBase.h>
#include <winerror.h>
#include <WinUser.h>

#include "constants.hpp"
#include "../detours/detours.h"
#include "dlldata.hpp"

PFN_SET_WINDOW_COMPOSITION_ATTRIBUTE Hook::SetWindowCompositionAttribute =
	reinterpret_cast<PFN_SET_WINDOW_COMPOSITION_ATTRIBUTE>(GetProcAddress(GetModuleHandle(L"user32.dll"), "SetWindowCompositionAttribute"));
std::mutex Hook::m_initDoneLock;
bool Hook::m_initDone = false;
const HWND Hook::m_TTBMsgWnd = FindWindow(WORKER_WINDOW, WORKER_WINDOW);
const unsigned int Hook::RequestAttributeRefresh = RegisterWindowMessage(L"TTBHook_RequestAttributeRefresh");

BOOL Hook::SetWindowCompositionAttributeDetour(HWND hWnd, const WINDOWCOMPOSITIONATTRIBDATA *data)
{
	if (data->Attrib == WCA_ACCENT_POLICY && SendMessage(m_TTBMsgWnd, RequestAttributeRefresh, 0, reinterpret_cast<LPARAM>(hWnd)))
	{
		return TRUE;
	}

	return SetWindowCompositionAttribute(hWnd, data);
}

LRESULT Hook::CallWndProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION)
	{
		std::lock_guard guard(m_initDoneLock);
		if (!m_initDone)
		{
			// TODO: error handling using OutputDebugString
			DetourTransactionBegin();
			DetourUpdateThread(GetCurrentThread());
			DetourAttach(reinterpret_cast<void **>(&SetWindowCompositionAttribute), reinterpret_cast<void *>(SetWindowCompositionAttributeDetour));
			DetourTransactionCommit();

			m_initDone = true;
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
