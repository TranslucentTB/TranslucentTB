#pragma once
#ifndef WIN32_HPP
#define WIN32_HPP

#include <cstdbool>
#include <cstdint>
#include <winbase.h>
#include <windef.h>
#include <winerror.h>
#include <winnt.h>

#include "ttberror.hpp"

namespace user32 {

	typedef bool(WINAPI *pSetWindowCompositionAttribute)(HWND, swca::WINCOMPATTRDATA *);
	static const pSetWindowCompositionAttribute SetWindowCompositionAttribute = reinterpret_cast<pSetWindowCompositionAttribute>(GetProcAddress(GetModuleHandle(L"user32.dll"), "SetWindowCompositionAttribute"));

}

namespace ntdll {

	// Importing a driver-specific function because it's the easiest way to acquire the current OS version without being lied to
	typedef NTSTATUS(__stdcall *pRtlGetVersion)(PRTL_OSVERSIONINFOW);
	static const pRtlGetVersion RtlGetVersion = reinterpret_cast<pRtlGetVersion>(GetProcAddress(GetModuleHandle(L"ntdll.dll"), "RtlGetVersion"));

}

namespace win32 {

	bool IsAtLeastBuild(const uint32_t &buildNumber)
	{
		RTL_OSVERSIONINFOW versionInfo;
		if (ntdll::RtlGetVersion && Error::Handle(HRESULT_FROM_NT(ntdll::RtlGetVersion(&versionInfo)), Error::Level::Log, L"Error obtaining version info."))
		{
			return versionInfo.dwBuildNumber >= buildNumber;
		}
		else
		{
			return false;
		}
	}

}

#endif // !WIN32_HPP
