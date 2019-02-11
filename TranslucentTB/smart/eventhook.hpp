#pragma once
#include "arch.h"
#include <functional>
#include <unordered_map>
#include <windef.h>
#include <WinUser.h>

#include "../windows/window.hpp"

class EventHook {
public:
	using callback_t = std::function<void(DWORD, Window, LONG, LONG, DWORD, DWORD)>;
private:
	HWINEVENTHOOK m_Handle;

	// As function because static initialization order.
	inline static std::unordered_map<HWINEVENTHOOK, callback_t> &GetMap()
	{
		static std::unordered_map<HWINEVENTHOOK, callback_t> map;
		return map;
	}

	static void CALLBACK RawHookCallback(HWINEVENTHOOK hook, DWORD event, HWND window, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime);

public:
	inline EventHook(HWINEVENTHOOK handle) : m_Handle(handle) { }
	inline EventHook(DWORD event, callback_t callback, DWORD idProcess = 0, DWORD idThread = 0, DWORD flags = WINEVENT_OUTOFCONTEXT) : EventHook(event, event, callback, idProcess, idThread, flags) { }
	EventHook(DWORD min, DWORD max, callback_t callback, DWORD idProcess = 0, DWORD idThread = 0, DWORD flags = WINEVENT_OUTOFCONTEXT);

	inline EventHook(const EventHook &) = delete;
	inline EventHook &operator =(const EventHook &) = delete;

	~EventHook();
};