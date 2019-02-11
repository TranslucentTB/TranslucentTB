#include "eventhook.hpp"

#include "../ttblog.hpp"

void CALLBACK EventHook::RawHookCallback(HWINEVENTHOOK hook, DWORD event, HWND window, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime)
{
	GetMap()[hook](event, window, idObject, idChild, dwEventThread, dwmsEventTime);
}

EventHook::EventHook(DWORD min, DWORD max, callback_t callback, DWORD idProcess, DWORD idThread, DWORD flags)
{
	m_Handle = SetWinEventHook(min, max, nullptr, RawHookCallback, idProcess, idThread, flags & ~WINEVENT_INCONTEXT);
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