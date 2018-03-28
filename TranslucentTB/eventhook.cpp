#include "eventhook.hpp"
#include <WinUser.h>

EventHook::EventHook(const HWINEVENTHOOK &handle)
{
	m_Handle = handle;
}

EventHook::EventHook(const DWORD &min, const DWORD &max, const std::function<void(HWINEVENTHOOK, DWORD, HWND, LONG, LONG, DWORD, DWORD)> &callback, const DWORD &flags, const HMODULE &hMod, const DWORD &idProcess, const DWORD &idThread)
{
	m_Handle = SetWinEventHook(min, max, hMod, *callback.target<WINEVENTPROC>(), idProcess, idThread, flags);
}

EventHook::~EventHook()
{
	UnhookWinEvent(m_Handle);
}
