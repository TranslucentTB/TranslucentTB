#pragma once
#include "arch.h"
#include <optional>
#include <windef.h>
#include <WinBase.h>
#include <wil/resource.h>
#include "winrt.hpp"
#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.Foundation.h>

#include "../ProgramLog/error/win32.hpp"

class StartupManager {
private:
	// TODO: replace with std::binary_semaphore
	mutable wil::unique_semaphore m_TaskLock;
	winrt::Windows::ApplicationModel::StartupTask m_StartupTask;

	winrt::Windows::ApplicationModel::StartupTask GetTaskSafe() const noexcept;

public:
	inline StartupManager() noexcept : m_TaskLock(1, 1), m_StartupTask(nullptr) { }

	winrt::Windows::Foundation::IAsyncOperation<bool> AcquireTask();

	std::optional<winrt::Windows::ApplicationModel::StartupTaskState> GetState() const;
	winrt::Windows::Foundation::IAsyncAction Enable();
	void Disable();

};
