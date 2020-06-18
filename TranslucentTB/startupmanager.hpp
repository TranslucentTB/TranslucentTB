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
	mutable wil::srwlock m_TaskLock;
	winrt::Windows::ApplicationModel::StartupTask m_StartupTask;

	winrt::Windows::ApplicationModel::StartupTask GetTaskSafe() const noexcept;

public:
	inline StartupManager() noexcept : m_StartupTask(nullptr) { }

	winrt::Windows::Foundation::IAsyncAction AcquireTask();

	std::optional<winrt::Windows::ApplicationModel::StartupTaskState> GetState() const;
	winrt::Windows::Foundation::IAsyncAction Enable();
	void Disable();

};
