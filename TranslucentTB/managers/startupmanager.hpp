#pragma once
#include <optional>
#include "winrt.hpp"
#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.Foundation.h>

class StartupManager {
private:
	winrt::Windows::ApplicationModel::StartupTask m_StartupTask;

public:
	inline StartupManager() noexcept : m_StartupTask(nullptr) { }

	winrt::Windows::Foundation::IAsyncOperation<bool> AcquireTask();

	std::optional<winrt::Windows::ApplicationModel::StartupTaskState> GetState() const;
	winrt::Windows::Foundation::IAsyncAction Enable();
	void Disable();
	winrt::Windows::Foundation::IAsyncAction OpenSettingsPage();
};
