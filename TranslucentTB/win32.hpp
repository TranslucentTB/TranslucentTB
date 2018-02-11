#pragma once
#ifndef WIN32_HPP
#define WIN32_HPP

#include <comdef.h>
#include <cstdbool>
#include <cstdint>
#include <cwchar>
#include <Shlwapi.h>
#include <winbase.h>
#include <winreg.h>

#include "swcadata.hpp"

namespace user32 {

	typedef bool(WINAPI *pSetWindowCompositionAttribute)(HWND, swca::WINCOMPATTRDATA *);
	static pSetWindowCompositionAttribute SetWindowCompositionAttribute = reinterpret_cast<pSetWindowCompositionAttribute>(GetProcAddress(GetModuleHandle(L"user32.dll"), "SetWindowCompositionAttribute"));

}

namespace ntdll {

	// Importing a driver-specific function because it's the easiest way to acquire the current OS version without being lied to
	typedef NTSTATUS(__stdcall *pRtlGetVersion)(PRTL_OSVERSIONINFOW);
	static pRtlGetVersion RtlGetVersion = reinterpret_cast<pRtlGetVersion>(GetProcAddress(GetModuleHandle(L"ntdll.dll"), "RtlGetVersion"));

}

namespace win32 {

	bool GetStartupState()
	{
		// SUCCEEDED macro considers ERROR_FILE_NOT_FOUND as succeeded ???
		return RegGetValue(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", App::NAME, RRF_RT_REG_SZ, NULL, NULL, NULL) == ERROR_SUCCESS;
	}

	void SetStartupState(bool state)
	{
		HKEY hkey;
		if (SUCCEEDED(RegCreateKey(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", &hkey))) //Creates a key
		{
			if (state)
			{
				HMODULE hModule = GetModuleHandle(NULL);
				wchar_t path[MAX_PATH];
				GetModuleFileName(hModule, path, MAX_PATH);
				PathQuoteSpaces(path);

				RegSetValueEx(hkey, App::NAME, 0, REG_SZ, reinterpret_cast<BYTE *>(path), wcslen(path) * sizeof(wchar_t));
			}
			else
			{
				RegDeleteValue(hkey, App::NAME);
			}
			RegCloseKey(hkey);
		}
	}

	bool IsAtLeastBuild(uint32_t buildNumber)
	{
		if (ntdll::RtlGetVersion)
		{
			RTL_OSVERSIONINFOW versionInfo;
			ntdll::RtlGetVersion(&versionInfo);
			return versionInfo.dwBuildNumber >= buildNumber;
		}
		else
		{
			return false;
		}
	}

}

#endif // !WIN32_HPP
