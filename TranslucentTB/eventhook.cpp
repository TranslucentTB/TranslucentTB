#include "eventhook.hpp"

#include "ttblog.hpp"

void CALLBACK EventHook::RawHookCallback(HWINEVENTHOOK hook, DWORD event, HWND window, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime)
{
	GetMap()[hook](event, window, idObject, idChild, dwEventThread, dwmsEventTime);
}

EventHook::EventHook(DWORD min, DWORD max, callback_t callback, DWORD flags, HMODULE hMod, DWORD idProcess, DWORD idThread)
{
	m_Handle = SetWinEventHook(min, max, hMod, RawHookCallback, idProcess, idThread, flags);
	if (m_Handle)
	{
		GetMap()[m_Handle] = std::move(callback);
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