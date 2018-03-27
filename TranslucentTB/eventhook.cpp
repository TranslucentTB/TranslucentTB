#include "eventhook.hpp"
#include <WinUser.h>

EventHook::EventHook(HWINEVENTHOOK handle)
{
	m_Handle = handle;
}

EventHook::EventHook(DWORD min, DWORD max, std::function<void(HWINEVENTHOOK, DWORD, HWND, LONG, LONG, DWORD, DWORD)> callback, DWORD flags, HMODULE hMod, DWORD idProcess, DWORD idThread)
{
	m_Handle = SetWinEventHook(min, max, hMod, *callback.target<WINEVENTPROC>(), idProcess, idThread, flags);
}

EventHook::~EventHook()
{
	UnhookWinEvent(m_Handle);
}
