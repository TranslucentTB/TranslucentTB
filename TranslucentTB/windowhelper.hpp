#pragma once
#ifndef WINDOWHELPER_HPP
#define WINDOWHELPER_HPP

#include <atlbase.h>
#include <combaseapi.h>
#include <dwmapi.h>
#include <psapi.h>
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
			if (Config::VERBOSE)
			{
				Error::Handle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Log, L"Getting title of a window failed.");
			}
			return L"";
		}

		return windowTitleBuffer.data();
	}

	std::wstring GetWindowClass(const HWND &hwnd)
	{
		wchar_t className[MAX_PATH];
		int result = GetClassName(hwnd, className, _countof(className));

		if (!result)
		{
			if (Config::VERBOSE)
			{
				Error::Handle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Log, L"Getting class name of a window failed.");
			}
			return L"";
		}

		return className;
	}

	std::wstring GetWindowFile(const HWND &hwnd)
	{
		DWORD ProcessId;
		GetWindowThreadProcessId(hwnd, &ProcessId);

		wchar_t exeName_path[MAX_PATH];
		DWORD result = GetModuleFileNameEx(OpenProcess(PROCESS_QUERY_INFORMATION, false, ProcessId), NULL, exeName_path, _countof(exeName_path));

		if (!result)
		{
			if (Config::VERBOSE)
			{
				Error::Handle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Log, L"Getting file name of a window failed.");
			}
			return L"";
		}

		return PathFindFileName(exeName_path);
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
			if (Config::VERBOSE)
			{
				Error::Handle(result, Error::Level::Log, L"Verifying if a window is on the current virtual desktop failed.");
			}
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
			if (Config::VERBOSE)
			{
				Error::Handle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Log, L"Getting placement of a window failed.");
			}
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
			if (Config::VERBOSE)
			{
				Error::Handle(status, Error::Level::Log, L"Getting cloaked status of a window failed.");
			}
			return false;
		}

		return cloaked;
	}

}

#endif // !WINDOWHELPER_HPP