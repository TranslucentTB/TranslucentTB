#pragma once
#ifdef STORE
#include <winrt/Windows.ApplicationModel.h>

// IMPORTANT IMPORTANT: Don't use preprocessors directives with as_then
#define as_then(x) Completed([=](const winrt::Windows::Foundation::IAsyncOperation<Autostart::StartupState> &task, winrt::Windows::Foundation::AsyncStatus) { const Autostart::StartupState result = task.GetResults(); x })
#else
#include <pplawait.h>

// IMPORTANT IMPORTANT: Don't use preprocessors directives with as_then
#define as_then(x) then([=](const Autostart::StartupState &result) x)
#endif

class Autostart {

private:
#ifdef STORE
	using action_t = winrt::Windows::Foundation::IAsyncAction;

	template<class T>
	using operation_t = winrt::Windows::Foundation::IAsyncOperation<T>;
#else
	using action_t = concurrency::task<void>;

	template<typename T>
	using operation_t = concurrency::task<T>;
#endif

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

	static operation_t<StartupState> GetStartupState();
	static action_t SetStartupState(StartupState state);
};