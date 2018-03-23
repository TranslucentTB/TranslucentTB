#include "win32.hpp"
#include "arch.h"
#include <synchapi.h>
#include <winerror.h>
#include <winnt.h>
#include <wrl/wrappers/corewrappers.h>

#include "app.hpp"
#include "ttberror.hpp"

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
	HANDLE event_handle = CreateEvent(NULL, TRUE, FALSE, App::ID.c_str());
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
