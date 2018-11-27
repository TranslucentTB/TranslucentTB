#pragma once
#include "arch.h"
#include <functional>
#include <unordered_map>
#include <windef.h>
#include <WinUser.h>

#include "window.hpp"

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
	EventHook(DWORD min, DWORD max, callback_t callback, DWORD flags, HMODULE hMod = NULL, DWORD idProcess = 0, DWORD idThread = 0);

	inline EventHook(const EventHook &) = delete;
	inline EventHook &operator =(const EventHook &) = delete;

	~EventHook();
};