#pragma once
#include <winrt/Windows.ApplicationModel.h>

class Autostart {

public:
	using StartupState = winrt::Windows::ApplicationModel::StartupTaskState;

	static winrt::Windows::Foundation::IAsyncOperation<StartupState> GetStartupState();
	static winrt::Windows::Foundation::IAsyncAction SetStartupState(StartupState state);
};