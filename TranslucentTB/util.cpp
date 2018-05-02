#include "util.hpp"
#include "arch.h"
#include <atlbase.h>
#include <PathCch.h>
#include <processthreadsapi.h>
#include <ShlObj.h>
#include <ShObjIdl.h>
#include <synchapi.h>
#include <WinBase.h>
#include <winerror.h>
#include <wrl/wrappers/corewrappers.h>

#include "app.hpp"
#include "autofree.hpp"
#include "../CPicker/CColourPicker.hpp"
#include "clipboardcontext.hpp"
#include "window.hpp"
#include "ttberror.hpp"

void Util::CopyToClipboard(const std::wstring &text)
{
	ClipboardContext context;
	if (!context)
	{
		ErrorHandle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Error, L"Failed to open clipboard.");
		return;
	}

	if (!EmptyClipboard())
	{
		ErrorHandle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Error, L"Failed to empty clipboard.");
		return;
	}

	const size_t url_size = text.length() + 1;
	AutoFree::Global<wchar_t> data(reinterpret_cast<wchar_t *>(GlobalAlloc(GMEM_FIXED, url_size * sizeof(wchar_t))));
	if (!data)
	{
		ErrorHandle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Error, L"Failed to allocate memory for the clipboard.");
		return;
	}

	wcscpy_s(data, url_size, text.c_str());

	if (!SetClipboardData(CF_UNICODETEXT, data))
	{
		ErrorHandle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Error, L"Failed to copy data to clipboard.");
		return;
	}
}

void Util::EditFile(std::wstring file)
{
	// WinAPI reeeeeeeeeeeeeeeeeeeeeeeeee
	AutoFree::CoTaskMem<wchar_t> system32;
	if (!ErrorHandle(SHGetKnownFolderPath(FOLDERID_System, KF_FLAG_DEFAULT, NULL, &system32), Error::Level::Error, L"Failed to determine System32 folder location!"))
	{
		return;
	}

	AutoFree::Local<wchar_t> notepad;
	if (!ErrorHandle(PathAllocCombine(system32, L"notepad.exe", PATHCCH_ALLOW_LONG_PATHS, &notepad), Error::Level::Error, L"Failed to determine Notepad location!"))
	{
		return;
	}

	std::wstring str_notepad(notepad);

	QuoteSpaces(str_notepad);
	QuoteSpaces(file);
	std::wstring path = str_notepad + L" " + file;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
	STARTUPINFO si = { sizeof(si) };
#pragma clang diagnostic pop

	PROCESS_INFORMATION pi;
	// Not using lpApplicationName here because if someone has set a redirect to another editor it doesn't works. (eg Notepad2)
	if (CreateProcess(NULL, path.data(), NULL, NULL, FALSE, NULL, NULL, NULL, &si, &pi))
	{
		namespace wrap = Microsoft::WRL::Wrappers;
		wrap::HandleT<wrap::HandleTraits::HANDLENullTraits> processHandle(pi.hProcess);
		wrap::HandleT<wrap::HandleTraits::HANDLENullTraits> threadHandle(pi.hThread);

		if (WaitForSingleObject(processHandle.Get(), INFINITE) == WAIT_FAILED)
		{
			ErrorHandle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Log, L"Failed to wait for Notepad close.");
		}
	}
	else
	{
		ErrorHandle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Error, L"Failed to start Notepad!");
	}
}

void Util::OpenLink(const std::wstring &link)
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
	SHELLEXECUTEINFO info = {
		sizeof(info),		// cbSize
		SEE_MASK_CLASSNAME,	// fMask
		NULL,				// hwnd
		L"open",			// lpVerb
		link.c_str(),		// lpFile
		NULL,				// lpParameters
		NULL,				// lpDirectory
		SW_SHOW,			// nShow
		nullptr,			// hInstApp
		nullptr,			// lpIDList
		L"https"			// lpClass
	};
#pragma clang diagnostic pop

	if (!ShellExecuteEx(&info))
	{
		std::wstring boxbuffer =
			L"Failed to open URL \"" + link + L"\"." +
			L"\n\n" + Error::ExceptionFromHRESULT(HRESULT_FROM_WIN32(GetLastError())) +
			L"\n\nCopy the URL to the clipboard?";

		if (MessageBox(NULL, boxbuffer.c_str(), (std::wstring(App::NAME) + L" - Error").c_str(), MB_ICONWARNING | MB_YESNO | MB_SETFOREGROUND) == IDYES)
		{
			CopyToClipboard(link);
		}
	}
}

std::thread Util::PickColor(uint32_t &color)
{
	std::thread t([&color]() {
		CColourPicker(color).CreateColourPicker();
	});

	t.detach();
	return t;
}

bool Util::IsStartVisible()
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
