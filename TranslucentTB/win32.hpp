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

#include "app.hpp"
#include "swcadata.hpp"
#include "ttberror.hpp"

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
		HRESULT error = RegGetValue(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", App::NAME, RRF_RT_REG_SZ, NULL, NULL, NULL);
		switch (error)
		{
			case ERROR_FILE_NOT_FOUND:
			{
				return false;
			}

			case ERROR_SUCCESS:
			{
				return true;
			}

			default:
			{
				Error::Handle(error, Error::Level::Log, L"Querying startup state failed.");
			}
		}
	}

	void SetStartupState(bool state)
	{
		HKEY hkey;
		HRESULT error = RegCreateKey(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", &hkey);
		if (Error::Handle(error, Error::Level::Error, L"Opening registry key failed!")) //Creates a key
		{
			if (state)
			{
				HMODULE hModule = GetModuleHandle(NULL);
				wchar_t path[MAX_PATH];
				GetModuleFileName(hModule, path, MAX_PATH);
				PathQuoteSpaces(path);

				error = RegSetValueEx(hkey, App::NAME, 0, REG_SZ, reinterpret_cast<BYTE *>(path), wcslen(path) * sizeof(wchar_t));
				Error::Handle(error, Error::Level::Error, L"Error while setting startup registry value!");
			}
			else
			{
				error = RegDeleteValue(hkey, App::NAME);
				Error::Handle(error, Error::Level::Error, L"Error while deleting startup registry value!");
			}
			error = RegCloseKey(hkey);
			Error::Handle(error, Error::Level::Log, L"Error closing registry key.");
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
