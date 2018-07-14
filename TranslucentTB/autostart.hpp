#pragma once
#ifdef STORE
#include <winrt/Windows.ApplicationModel.h>
#endif

class Autostart {

public:
#ifndef STORE
	enum class StartupState {
		Disabled,
		DisabledByUser,
		Enabled
	};
#else
	using StartupState = winrt::Windows::ApplicationModel::StartupTaskState;
#endif

	static StartupState GetStartupState();
	static void SetStartupState(const StartupState &state);
};