#include "util.hpp"
#include "arch.h"
#include <atlbase.h>
#include <PathCch.h>
#include <processthreadsapi.h>
#include <ShlObj.h>
#include <ShObjIdl.h>
#include <synchapi.h>
#include <thread>
#include <WinBase.h>
#include <winerror.h>
#include <wrl/wrappers/corewrappers.h>

#include "autofree.hpp"
#include "../CPicker/CColourPicker.hpp"
#include "window.hpp"
#include "ttberror.hpp"

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

	std::vector<wchar_t> buf(path.begin(), path.end());
	buf.push_back(0); // Null terminator

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
	STARTUPINFO si = { sizeof(si) };
#pragma clang diagnostic pop

	PROCESS_INFORMATION pi;
	// Not using lpApplicationName here because if someone has set a redirect to another editor it doesn't works. (eg Notepad2)
	if (CreateProcess(NULL, buf.data(), NULL, NULL, FALSE, NULL, NULL, NULL, &si, &pi))
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

void Util::PickColor(uint32_t &color)
{
	std::thread([&color]() {
		CColourPicker(color).CreateColourPicker();
	}).detach();
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
