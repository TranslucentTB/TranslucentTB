#pragma once
#include "arch.h"
#include <windef.h>
#include <WinUser.h>

class EventHook {

private:
	HWINEVENTHOOK m_Handle;

public:
	inline EventHook(const HWINEVENTHOOK &handle) : m_Handle(handle) { }
	EventHook(const DWORD &min, const DWORD &max, WINEVENTPROC callback, const DWORD &flags, const HMODULE &hMod = NULL, const DWORD &idProcess = 0, const DWORD &idThread = 0);

	inline EventHook(const EventHook &) = delete;
	inline EventHook &operator =(const EventHook &) = delete;

	~EventHook();
};