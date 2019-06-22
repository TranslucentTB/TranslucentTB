#pragma once
#include <winrt/Windows.ApplicationModel.h>

class Autostart {
private:
	static winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::ApplicationModel::StartupTask> GetApplicationStartupTask();

public:
	using StartupState = winrt::Windows::ApplicationModel::StartupTaskState;

	static winrt::Windows::Foundation::IAsyncOperation<StartupState> GetStartupState();
	static winrt::Windows::Foundation::IAsyncAction SetStartupState(StartupState state);
};