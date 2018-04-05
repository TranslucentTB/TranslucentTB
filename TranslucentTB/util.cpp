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

#include "AutoFree.hpp"
#include "../CPicker/CPickerDll.h"
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

uint32_t Util::PickColor(const uint32_t &color)
{
	const unsigned short a = (color & 0xFF000000) >> 24;
	const unsigned short r = (color & 0x00FF0000) >> 16;
	const unsigned short g = (color & 0x0000FF00) >> 8;
	const unsigned short b = (color & 0x000000FF);

	// Bet 5 bucks a british wrote this library
	CColourPicker picker(NULL, r, g, b, a, true);
	picker.CreateColourPicker();
	SColour newColor = picker.GetCurrentColour();

	return (newColor.a << 24) + (newColor.r << 16) + (newColor.g << 8) + newColor.b;
}

void Util::AddValuesToVectorByDelimiter(const std::wstring &delimiter, std::vector<std::wstring> &vector, std::wstring line)
{
	size_t pos;
	std::wstring value;

	// First lets remove the key
	if ((pos = line.find(delimiter)) != std::wstring::npos)
	{
		line.erase(0, pos + delimiter.length());
	}

	// Now iterate and add the values
	while ((pos = line.find(delimiter)) != std::wstring::npos)
	{
		value = Trim(line.substr(0, pos));
		vector.push_back(value);
		line.erase(0, pos + delimiter.length());
	}
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
