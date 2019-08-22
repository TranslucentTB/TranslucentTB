#include "eventhook.hpp"
#include "../../ProgramLog/error.hpp"

std::unordered_map<HWINEVENTHOOK, EventHook::callback_t> EventHook::s_HookMap;

void CALLBACK EventHook::RawHookCallback(HWINEVENTHOOK hook, DWORD event, HWND window, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime)
{
	s_HookMap.at(hook)(event, window, idObject, idChild, dwEventThread, dwmsEventTime);
}

EventHook::EventHook(DWORD min, DWORD max, callback_t callback, DWORD idProcess, DWORD idThread, DWORD flags) : m_Handle(SetWinEventHook(min, max, nullptr, RawHookCallback, idProcess, idThread, flags & ~WINEVENT_INCONTEXT))
{
	if (m_Handle)
	{
		s_HookMap[m_Handle.get()] = std::move(callback);
	}
	else
	{
		MessagePrint(spdlog::level::warn, L"Failed to create a Windows event hook.");
	}
}
