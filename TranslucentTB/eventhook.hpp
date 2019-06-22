#pragma once
#include "arch.h"
#include <functional>
#include <unordered_map>
#include <windef.h>
#include <WinUser.h>
#include <wil/resource.h>

#include "window.hpp"

class EventHook {
private:
	using callback_t = std::function<void(DWORD, Window, LONG, LONG, DWORD, DWORD)>;
	
	// As function because static initialization order.
	inline static std::unordered_map<HWINEVENTHOOK, callback_t> &GetMap()
	{
		static std::unordered_map<HWINEVENTHOOK, callback_t> map;
		return map;
	}

	static void CALLBACK RawHookCallback(HWINEVENTHOOK hook, DWORD event, HWND window, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime);

	wil::unique_hwineventhook m_Handle;

public:
	EventHook(DWORD min, DWORD max, callback_t callback, DWORD idProcess = 0, DWORD idThread = 0, DWORD flags = WINEVENT_OUTOFCONTEXT);

	inline EventHook(DWORD event, callback_t callback, DWORD idProcess = 0, DWORD idThread = 0, DWORD flags = WINEVENT_OUTOFCONTEXT) : EventHook(event, event, std::move(callback), idProcess, idThread, flags)
	{ }

	inline EventHook(EventHook &&other)
	{
		std::swap(m_Handle, other.m_Handle);
	}

	inline EventHook &operator =(EventHook &&other)
	{
		if (this != &other)
		{
			std::swap(m_Handle, other.m_Handle);
		}
	}

	inline ~EventHook()
	{
		GetMap().erase(m_Handle.get());
	}
};