#pragma once
#include <ppltasks.h>
#include <winrt/Windows.ApplicationModel.h>

class Autostart {

public:
	using StartupState = winrt::Windows::ApplicationModel::StartupTaskState;

	static concurrency::task<StartupState> GetStartupState();
	static concurrency::task<void> SetStartupState(const StartupState &state);
};