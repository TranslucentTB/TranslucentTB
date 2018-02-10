#pragma once
#ifndef WIN32_HPP

#include <cstdbool>
#include <windef.h>

#include "compositiondata.hpp"

namespace user32 {
	typedef bool(WINAPI *pSetWindowCompositionAttribute)(HWND, WINCOMPATTRDATA *);
	static pSetWindowCompositionAttribute SetWindowCompositionAttribute = reinterpret_cast<pSetWindowCompositionAttribute>(GetProcAddress(GetModuleHandle(L"user32.dll"), "SetWindowCompositionAttribute"));
}

namespace ntdll {
	// Importing a driver-specific function because it's the easiest way to acquire the current OS version without being lied to
	typedef NTSTATUS(__stdcall *pRtlGetVersion)(PRTL_OSVERSIONINFOW);
	static pRtlGetVersion RtlGetVersion = reinterpret_cast<pRtlGetVersion>(GetProcAddress(GetModuleHandle(L"ntdll.dll"), "RtlGetVersion"));
}

#endif // !WIN32_HPP
