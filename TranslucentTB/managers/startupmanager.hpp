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

	wf::IAsyncOperation<bool> AcquireTask();

	std::optional<winrt::Windows::ApplicationModel::StartupTaskState> GetState() const;
	wf::IAsyncAction Enable();
	void Disable();
	static void OpenSettingsPage();

	inline explicit operator bool() const noexcept
	{
		return m_StartupTask != nullptr;
	}
};
