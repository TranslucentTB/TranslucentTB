#pragma once
#ifndef AUTOSTART_HPP
#define AUTOSTART_HPP

#ifndef STORE
#include <cwchar>
#include <Shlwapi.h>
#include <WinBase.h>
#include <winreg.h>

#include "app.hpp"
#else
#define _HIDE_GLOBAL_ASYNC_STATUS
#include <roapi.h>
#include <Windows.ApplicationModel.h>
#include <wrl/client.h>

#include "SynchronousOperation.hpp"
#include "UWP.hpp"
#endif

#include "ttberror.hpp"

namespace Autostart {

	enum class StartupState {
#ifndef STORE
		Disabled,
		DisabledByUser,
		DisabledByPolicy,
		Enabled
#else
		Disabled = ABI::Windows::ApplicationModel::StartupTaskState_Disabled,
		DisabledByUser = ABI::Windows::ApplicationModel::StartupTaskState_DisabledByUser,
		DisabledByPolicy = ABI::Windows::ApplicationModel::StartupTaskState_DisabledByPolicy,
		Enabled = ABI::Windows::ApplicationModel::StartupTaskState_Enabled
#endif
	};

	StartupState GetStartupState()
	{
#ifndef STORE
		LRESULT error = RegGetValue(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", App::NAME.c_str(), RRF_RT_REG_SZ, NULL, NULL, NULL);
		if (error == ERROR_FILE_NOT_FOUND)
		{
			return StartupState::Disabled;
		}
		else if (error == ERROR_SUCCESS)
		{
			uint8_t status[12];
			DWORD size = sizeof(status);
			error = RegGetValue(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartupApproved\\Run", App::NAME.c_str(), RRF_RT_REG_BINARY, NULL, &status, &size);
			if (error != ERROR_FILE_NOT_FOUND && Error::Handle(HRESULT_FROM_WIN32(error), Error::Level::Log, L"Querying startup disable state failed.") && status[0] == 3)
			{
				return StartupState::DisabledByUser;
			}
			else
			{
				return StartupState::Enabled;
			}
		}
		else
		{
			Error::Handle(HRESULT_FROM_WIN32(error), Error::Level::Log, L"Querying startup state failed.");
			return StartupState::Disabled;
		}
#else
		auto task = UWP::GetApplicationStartupTask();
		if (!task)
		{
			return StartupState::Disabled;
		}

		ABI::Windows::ApplicationModel::StartupTaskState state;
		if (!Error::Handle(task->get_State(&state), Error::Level::Log, L"Could not retrieve startup task state"))
		{
			return StartupState::Disabled;
		}
		else
		{
			return static_cast<StartupState>(state);
		}
#endif
	}

	void SetStartupState(const StartupState &state)
	{
#ifndef STORE
		HKEY hkey;
		LRESULT error = RegCreateKey(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", &hkey);
		if (Error::Handle(HRESULT_FROM_WIN32(error), Error::Level::Error, L"Opening registry key failed!")) //Creates a key
		{
			if (state == StartupState::Enabled)
			{
				HMODULE hModule = GetModuleHandle(NULL);
				wchar_t path[MAX_PATH];
				GetModuleFileName(hModule, path, MAX_PATH);
				PathQuoteSpaces(path);

				error = RegSetValueEx(hkey, App::NAME.c_str(), 0, REG_SZ, reinterpret_cast<BYTE *>(path), wcslen(path) * sizeof(wchar_t));
				Error::Handle(HRESULT_FROM_WIN32(error), Error::Level::Error, L"Error while setting startup registry value!");
			}
			else if (state == StartupState::Disabled)
			{
				error = RegDeleteValue(hkey, App::NAME.c_str());
				Error::Handle(HRESULT_FROM_WIN32(error), Error::Level::Error, L"Error while deleting startup registry value!");
			}
			error = RegCloseKey(hkey);
			Error::Handle(HRESULT_FROM_WIN32(error), Error::Level::Log, L"Error closing registry key.");
		}
#else
		using namespace ABI::Windows;
		using namespace Microsoft::WRL;

		auto task = UWP::GetApplicationStartupTask();
		if (!task)
		{
			return;
		}

		if (state == StartupState::Enabled)
		{
			ComPtr<Foundation::IAsyncOperation<ApplicationModel::StartupTaskState>> operation;
			if (!Error::Handle(task->RequestEnableAsync(&operation), Error::Level::Log, L"Could not start setting of startup task state"))
			{
				return;
			}

			ApplicationModel::StartupTaskState new_state;
			Error::Handle(SynchronousOperation<ApplicationModel::StartupTaskState>(operation.Get()).GetResults(&new_state), Error::Level::Log, L"Could not set new startup task state");
		}
		else if (state == StartupState::Disabled)
		{
			Error::Handle(task->Disable(), Error::Level::Log, L"Could not disable startup task state");
		}
#endif
	}

}

#endif // !AUTOSTART_HPP
