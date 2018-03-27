#pragma once
#include "arch.h"
#include <functional>
#include <windef.h>

class EventHook {

private:
	HWINEVENTHOOK m_Handle;

public:
	EventHook(HWINEVENTHOOK handle);
	EventHook(DWORD min, DWORD max, std::function<void(HWINEVENTHOOK, DWORD, HWND, LONG, LONG, DWORD, DWORD)> callback, DWORD flags, HMODULE hMod = NULL, DWORD idProcess = 0, DWORD idThread = 0);

	inline EventHook(const EventHook &) = delete;
	inline EventHook &operator =(const EventHook &) = delete;

	~EventHook();
};