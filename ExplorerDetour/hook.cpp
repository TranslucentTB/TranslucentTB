#include "hook.hpp"
#include <WinBase.h>
#include <winerror.h>
#include <WinUser.h>

#include "../detours/detours.h"
#include "dlldata.hpp"

Hook::pSWCA Hook::SetWindowCompositionAttribute = reinterpret_cast<pSWCA>(GetProcAddress(GetModuleHandle(L"user32.dll"), "SetWindowCompositionAttribute"));
std::mutex Hook::m_taskbarsLock;
std::unordered_set<HWND> Hook::m_taskbars;
std::mutex Hook::m_excludedTaskbarsLock;
std::unordered_set<HWND> Hook::m_excludedTaskbars;
std::mutex Hook::m_initDoneLock;
bool Hook::m_initDone;
unsigned int Hook::m_PushWindowMsg = RegisterWindowMessage(L"TTBHook_PushWindow");
unsigned int Hook::m_ExcludeWindowMsg = RegisterWindowMessage(L"TTBHook_ExcludeWindow");
unsigned int Hook::m_IncludeWindowMsg = RegisterWindowMessage(L"TTBHook_IncludeWindow");

BOOL Hook::SetWindowCompositionAttributeDetour(HWND hWnd, swca::WINCOMPATTRDATA *data)
{
	if (data->nAttribute == swca::WindowCompositionAttribute::WCA_ACCENT_POLICY)
	{
		std::lock_guard guard(m_taskbarsLock);
		if (m_taskbars.count(hWnd) != 0 && m_excludedTaskbars.count(hWnd) == 0)
		{
			return TRUE;
		}
	}

	return SetWindowCompositionAttribute(hWnd, data);
}

LRESULT Hook::CallWndProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION)
	{
		{
			std::lock_guard guard(m_initDoneLock);
			if (!m_initDone)
			{
				// TODO: error handling
				DetourTransactionBegin();
				DetourUpdateThread(GetCurrentThread());
				DetourAttach(reinterpret_cast<void **>(&SetWindowCompositionAttribute), reinterpret_cast<void *>(SetWindowCompositionAttributeDetour));
				DetourTransactionCommit();

				m_initDone = true;
			}
		}

		const CWPSTRUCT *cwp = reinterpret_cast<CWPSTRUCT *>(lParam);
		if (cwp->message == m_PushWindowMsg)
		{
			std::lock_guard guard(m_taskbarsLock);
			m_taskbars.insert(cwp->hwnd);
		}
		else if (cwp->message == m_ExcludeWindowMsg)
		{
			std::lock_guard guard(m_excludedTaskbarsLock);
			m_excludedTaskbars.insert(cwp->hwnd);
		}
		else if (cwp->message == m_IncludeWindowMsg)
		{
			std::lock_guard guard(m_excludedTaskbarsLock);
			m_excludedTaskbars.erase(cwp->hwnd);
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

	SendMessage(taskbar, m_PushWindowMsg, 0, 0);

	return { hook, S_OK };
}

void Hook::ExcludeTaskbar(HWND taskbar)
{
	SendMessage(taskbar, m_ExcludeWindowMsg, 0, 0);
}

void Hook::IncludeTaskbar(HWND taskbar)
{
	SendMessage(taskbar, m_IncludeWindowMsg, 0, 0);
}
