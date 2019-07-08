#include "autostart.hpp"
#include "arch.h"
#include <cstdint>
#include <string>
#include <vector>
#include <windef.h>
#include <WinBase.h>
#include <winerror.h>
#include <winreg.h>

#include "common.hpp"
#include "registrykey.hpp"
#include "ttberror.hpp"
#include "win32.hpp"

winrt::Windows::Foundation::IAsyncOperation<Autostart::StartupState> Autostart::GetStartupState()
{
	co_await winrt::resume_background();

	LRESULT error = RegGetValue(HKEY_CURRENT_USER, LR"(SOFTWARE\Microsoft\Windows\CurrentVersion\Run)", NAME, RRF_RT_REG_SZ, NULL, NULL, NULL);
	if (error == ERROR_FILE_NOT_FOUND)
	{
		co_return StartupState::Disabled;
	}
	else if (error == ERROR_SUCCESS)
	{
		uint8_t status[12];
		DWORD size = sizeof(status);
		error = RegGetValue(HKEY_CURRENT_USER, LR"(SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\StartupApproved\Run)", NAME, RRF_RT_REG_BINARY, NULL, &status, &size);
		if (error != ERROR_FILE_NOT_FOUND && ErrorHandle(HRESULT_FROM_WIN32(error), Error::Level::Log, L"Querying startup disable state failed.") && status[0] == 3)
		{
			co_return StartupState::DisabledByUser;
		}
		else
		{
			co_return StartupState::Enabled;
		}
	}
	else
	{
		ErrorHandle(HRESULT_FROM_WIN32(error), Error::Level::Log, L"Querying startup state failed.");
		co_return StartupState::Disabled;
	}
}

winrt::Windows::Foundation::IAsyncAction Autostart::SetStartupState(const StartupState &state)
{
	co_await winrt::resume_background();

	registry_key key = open_key(HKEY_CURRENT_USER, LR"(SOFTWARE\Microsoft\Windows\CurrentVersion\Run)");
	if (key)
	{
		if (state == StartupState::Enabled)
		{
			const std::wstring exeLocation = L'"' + win32::GetExeLocation() + L'"';

			const LRESULT error = RegSetValueEx(key.get(), NAME, 0, REG_SZ, reinterpret_cast<const BYTE *>(exeLocation.c_str()), exeLocation.length() * sizeof(wchar_t));
			ErrorHandle(HRESULT_FROM_WIN32(error), Error::Level::Error, L"Error while setting startup registry value!");
		}
		else if (state == StartupState::Disabled)
		{
			ErrorHandle(HRESULT_FROM_WIN32(RegDeleteValue(key.get(), NAME)), Error::Level::Error, L"Error while deleting startup registry value!");
		}
		else
		{
			throw std::invalid_argument("Can only set state to enabled or disabled");
		}
	}
	else
	{
		LastErrorHandle(Error::Level::Error, L"Opening registry key failed!");
	}
}