#include "win32.hpp"
#include "arch.h"
#include <atlbase.h>
#include <PathCch.h>
#include <shellapi.h>
#include <ShlObj.h>
#include <ShObjIdl.h>
#include <synchapi.h>
#include <vector>
#include <WinBase.h>
#include <winerror.h>
#include <winnt.h>
#include <wrl/wrappers/corewrappers.h>

#include "autofree.hpp"
#include "../CPicker/CColourPicker.hpp"
#include "clipboardcontext.hpp"
#include "common.hpp"
#include "window.hpp"
#include "ttberror.hpp"

const user32::pSetWindowCompositionAttribute user32::SetWindowCompositionAttribute =
	reinterpret_cast<pSetWindowCompositionAttribute>(
		GetProcAddress(GetModuleHandle(L"user32.dll"), "SetWindowCompositionAttribute")
	);

std::wstring win32::m_ExeLocation;

const std::wstring &win32::GetExeLocation()
{
	if (m_ExeLocation.empty())
	{
		DWORD exeLocation_size = LONG_PATH;
		std::vector<wchar_t> exeLocation(exeLocation_size);
		if (!QueryFullProcessImageName(GetCurrentProcess(), 0, exeLocation.data(), &exeLocation_size))
		{
			LastErrorHandle(Error::Level::Fatal, L"Failed to determine executable location!");
		}
		else
		{
			m_ExeLocation = exeLocation.data();
		}
	}

	return m_ExeLocation;
}

bool win32::IsAtLeastBuild(const uint32_t &buildNumber)
{
	OSVERSIONINFOEX versionInfo;
	versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);
	versionInfo.dwBuildNumber = buildNumber;

	unsigned long long mask = VerSetConditionMask(0, VER_BUILDNUMBER, VER_GREATER_EQUAL);

	if (!VerifyVersionInfo(&versionInfo, VER_BUILDNUMBER, mask))
	{
		DWORD error = GetLastError();
		if (error != ERROR_OLD_WIN_VERSION)
		{
			ErrorHandle(HRESULT_FROM_WIN32(error), Error::Level::Log, L"Error obtaining version info.");
		}

		return false;
	}
	else
	{
		return true;
	}
}

bool win32::IsSingleInstance()
{
	HANDLE event_handle = CreateEvent(NULL, TRUE, FALSE, ID);
	LRESULT error = GetLastError();
	bool return_value;
	switch (error)
	{
	case ERROR_ALREADY_EXISTS:
		return_value = false;
		break;

	case ERROR_SUCCESS:
		return_value = true;
		break;

	default:
		ErrorHandle(HRESULT_FROM_WIN32(error), Error::Level::Error, L"Failed to open app handle!");
		return_value = true;
	}

	static Microsoft::WRL::Wrappers::Event event(event_handle); // RAII, the event automatically closes when we exit.
	return return_value;
}

bool win32::IsDirectory(const std::wstring &directory)
{
	DWORD attributes = GetFileAttributes(directory.c_str());
	return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY);
}

bool win32::FileExists(const std::wstring &file)
{
	DWORD attributes = GetFileAttributes(file.c_str());
	return attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY);
}

void win32::CopyToClipboard(const std::wstring &text)
{
	ClipboardContext context;
	if (!context)
	{
		LastErrorHandle(Error::Level::Error, L"Failed to open clipboard.");
		return;
	}

	if (!EmptyClipboard())
	{
		LastErrorHandle(Error::Level::Error, L"Failed to empty clipboard.");
		return;
	}

	const size_t url_size = text.length() + 1;
	AutoFree::Global<wchar_t> data(reinterpret_cast<wchar_t *>(GlobalAlloc(GMEM_FIXED, url_size * sizeof(wchar_t))));
	if (!data)
	{
		LastErrorHandle(Error::Level::Error, L"Failed to allocate memory for the clipboard.");
		return;
	}

	wcscpy_s(data, url_size, text.c_str());

	if (!SetClipboardData(CF_UNICODETEXT, data))
	{
		LastErrorHandle(Error::Level::Error, L"Failed to copy data to clipboard.");
		return;
	}
}

void win32::EditFile(const std::wstring &file)
{
	SHELLEXECUTEINFO info = {
		sizeof(info),									// cbSize
		SEE_MASK_CLASSNAME | SEE_MASK_NOCLOSEPROCESS,	// fMask
		NULL,											// hwnd
		L"open",										// lpVerb
		file.c_str(),									// lpFile
		NULL,											// lpParameters
		NULL,											// lpDirectory
		SW_SHOW,										// nShow
		nullptr,										// hInstApp
		nullptr,										// lpIDList
		L"txtfile"										// lpClass
	};

	if (ShellExecuteEx(&info))
	{
		if (WaitForSingleObject(info.hProcess, INFINITE) == WAIT_FAILED)
		{
			LastErrorHandle(Error::Level::Log, L"Failed to wait for text editor close.");
		}

		CloseHandle(info.hProcess);
	}
	else
	{
		std::wstring boxbuffer =
			L"Failed to open file \"" + file + L"\"." +
			L"\n\n" + Error::ExceptionFromHRESULT(HRESULT_FROM_WIN32(GetLastError())) +
			L"\n\nCopy the file location to the clipboard?";

		if (MessageBox(NULL, boxbuffer.c_str(), (std::wstring(NAME) + L" - Error").c_str(), MB_ICONWARNING | MB_YESNO | MB_SETFOREGROUND) == IDYES)
		{
			CopyToClipboard(file);
		}
	}
}

void win32::OpenLink(const std::wstring &link)
{
	SHELLEXECUTEINFO info = {
		sizeof(info),							// cbSize
		SEE_MASK_CLASSNAME,						// fMask
		NULL,									// hwnd
		L"open",								// lpVerb
		link.c_str(),							// lpFile
		NULL,									// lpParameters
		NULL,									// lpDirectory
		SW_SHOW,								// nShow
		nullptr,								// hInstApp
		nullptr,								// lpIDList
		link[4] == L's' ? L"https" : L"http"	// lpClass
	};

	if (!ShellExecuteEx(&info))
	{
		std::wstring boxbuffer =
			L"Failed to open URL \"" + link + L"\"." +
			L"\n\n" + Error::ExceptionFromHRESULT(HRESULT_FROM_WIN32(GetLastError())) +
			L"\n\nCopy the URL to the clipboard?";

		if (MessageBox(NULL, boxbuffer.c_str(), (std::wstring(NAME) + L" - Error").c_str(), MB_ICONWARNING | MB_YESNO | MB_SETFOREGROUND) == IDYES)
		{
			CopyToClipboard(link);
		}
	}
}

std::thread win32::PickColor(uint32_t &color)
{
	std::thread t([&color]()
	{
		CColourPicker(color).CreateColourPicker();
	});

	t.detach();
	return t;
}

bool win32::IsStartVisible()
{
	static CComPtr<IAppVisibility> app_visibility;
	static bool failed = false;

	if (!failed)
	{
		if (!app_visibility)
		{
			failed = !ErrorHandle(app_visibility.CoCreateInstance(CLSID_AppVisibility), Error::Level::Log, L"Initialization of IAppVisibility failed.");
		}
	}

	BOOL start_visible;
	if (failed || !ErrorHandle(app_visibility->IsLauncherVisible(&start_visible), Error::Level::Log, L"Checking start menu visibility failed."))
	{
		return false;
	}
	else
	{
		return start_visible;
	}
}