#pragma once
#ifndef WINDOWHELPER_HPP
#define WINDOWHELPER_HPP

#include <atlbase.h>
#include <combaseapi.h>
#include <dwmapi.h>
#include <Psapi.h>
#include <Shlwapi.h>
#include <ShObjIdl.h>
#include <string>
#include <vector>
#include <windef.h>
#include <winerror.h>
#include <WinUser.h>

#include "config.hpp"
#include "ttberror.hpp"

namespace WindowHelper {

	std::wstring GetWindowTitle(const HWND &hwnd)
	{
		int titleSize = GetWindowTextLength(hwnd) + 1; // For the null terminator
		std::vector<wchar_t> windowTitleBuffer(titleSize);
		int result = GetWindowText(hwnd, windowTitleBuffer.data(), titleSize);

		if (!result)
		{
			Error::Handle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Log, L"Getting title of a window failed.");
			return L"";
		}

		return windowTitleBuffer.data();
	}

	std::wstring GetWindowClass(const HWND &hwnd)
	{
		wchar_t className[256];
		int result = GetClassName(hwnd, className, 256);

		if (!result)
		{
			Error::Handle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Log, L"Getting class name of a window failed.");
			return L"";
		}

		return className;
	}

	std::wstring GetWindowFile(const HWND &hwnd)
	{
		DWORD ProcessId;
		GetWindowThreadProcessId(hwnd, &ProcessId);

		HANDLE processHandle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, ProcessId);

		if (!processHandle)
		{
			Error::Handle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Log, L"Getting process handle of a window failed.");
			return L"";
		}

		DWORD path_Size = LONG_PATH;
		std::vector<wchar_t> exeName_path(LONG_PATH);

		if (!QueryFullProcessImageName(processHandle, 0, exeName_path.data(), &path_Size))
		{
			Error::Handle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Log, L"Getting file name of a window failed.");

			if (!CloseHandle(processHandle))
			{
				if (Config::VERBOSE)
				{
					Error::Handle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Log, L"Closing process handle of a window failed.");
				}
			}

			return L"";
		}

		if (!CloseHandle(processHandle))
		{
			if (Config::VERBOSE)
			{
				Error::Handle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Log, L"Closing process handle of a window failed.");
			}
		}

		std::wstring exeName(exeName_path.data());
		return exeName.substr(exeName.find_last_of(L"/\\") + 1);
	}

	bool IsWindowOnCurrentDesktop(const HWND &hWnd)
	{
		static CComPtr<IVirtualDesktopManager> desktop_manager;
		static bool failed = false;

		if (failed)
		{
			return true;
		}
		else if (!desktop_manager)
		{
			failed = !Error::Handle(desktop_manager.CoCreateInstance(CLSID_VirtualDesktopManager), Error::Level::Log, L"Initialization of IVirtualDesktopManager failed.");
			if (failed)
			{
				return true;
			}
		}

		BOOL on_current_desktop;  // This must be a BOOL not a bool because Windows and C89 are equally stupid
		HRESULT result = desktop_manager->IsWindowOnCurrentVirtualDesktop(hWnd, &on_current_desktop);
		if (FAILED(result))
		{
			Error::Handle(result, Error::Level::Log, L"Verifying if a window is on the current virtual desktop failed.");
			return true;
		}

		return on_current_desktop;
	}

	bool IsWindowMaximised(const HWND &hWnd)
	{
		WINDOWPLACEMENT result = {};
		BOOL status = GetWindowPlacement(hWnd, &result);
		if (!status)
		{
			Error::Handle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Log, L"Getting placement of a window failed.");
			return false;
		}
		return result.showCmd == SW_MAXIMIZE;
	}

	bool IsWindowCloaked(const HWND &hWnd)
	{
		int cloaked;
		HRESULT status = DwmGetWindowAttribute(hWnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked));

		if (FAILED(status))
		{
			Error::Handle(status, Error::Level::Log, L"Getting cloaked status of a window failed.");
			return false;
		}

		return cloaked;
	}

}

#endif // !WINDOWHELPER_HPP