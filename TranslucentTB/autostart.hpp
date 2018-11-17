#pragma once
#include <winrt/Windows.ApplicationModel.h>

class Autostart {
private:
	static winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::ApplicationModel::StartupTask> __stdcall GetApplicationStartupTask();

public:
	using StartupState = winrt::Windows::ApplicationModel::StartupTaskState;

	static winrt::Windows::Foundation::IAsyncOperation<StartupState> __stdcall GetStartupState();
	static winrt::Windows::Foundation::IAsyncAction __stdcall SetStartupState(StartupState state);
};