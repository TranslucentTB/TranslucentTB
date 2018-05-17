#include "eventhook.hpp"
#include <WinUser.h>

#include "ttblog.hpp"

EventHook::EventHook(const DWORD &min, const DWORD &max, const std::function<void(HWINEVENTHOOK, DWORD, HWND, LONG, LONG, DWORD, DWORD)> &callback, const DWORD &flags, const HMODULE &hMod, const DWORD &idProcess, const DWORD &idThread)
{
	m_Handle = SetWinEventHook(min, max, hMod, *callback.target<WINEVENTPROC>(), idProcess, idThread, flags);
	if (!m_Handle)
	{
		Log::OutputMessage(L"Failed to create a Windows event hook.");
	}
}

EventHook::~EventHook()
{
	if (m_Handle)
	{
		if (!UnhookWinEvent(m_Handle))
		{
			Log::OutputMessage(L"Failed to delete a Windows event hook.");
		}
	}
}
