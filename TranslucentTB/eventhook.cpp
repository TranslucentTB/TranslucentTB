#include "eventhook.hpp"

#include "ttblog.hpp"

void CALLBACK EventHook::RawHookCallback(HWINEVENTHOOK hook, DWORD event, HWND window, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime)
{
	GetMap()[hook](event, window, idObject, idChild, dwEventThread, dwmsEventTime);
}

EventHook::EventHook(const DWORD &min, const DWORD &max, const callback_t &callback, const DWORD &flags, const HMODULE &hMod, const DWORD &idProcess, const DWORD &idThread)
{
	m_Handle = SetWinEventHook(min, max, hMod, RawHookCallback, idProcess, idThread, flags);
	if (m_Handle)
	{
		GetMap().emplace(m_Handle, callback);
	}
	else
	{
		Log::OutputMessage(L"Failed to create a Windows event hook.");
	}
}

EventHook::~EventHook()
{
	if (m_Handle)
	{
		GetMap().erase(m_Handle);
		if (!UnhookWinEvent(m_Handle))
		{
			Log::OutputMessage(L"Failed to delete a Windows event hook.");
		}
	}
}