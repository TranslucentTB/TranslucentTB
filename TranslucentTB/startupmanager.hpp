#pragma once
#include "arch.h"
#include <optional>
#include <string_view>
#include <synchapi.h>
#include <wil/resource.h>
#include "winrt.hpp"
#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>

#include "../ProgramLog/error/win32.hpp"

class StartupManager {
private:
	static constexpr std::wstring_view FAILED_TO_ENABLE = L"Failed to enable startup task.";

	mutable wil::srwlock m_TaskLock;
	winrt::Windows::ApplicationModel::StartupTask m_StartupTask;

	wil::slim_event_manual_reset m_TaskAcquiredEvent;

public:
	StartupManager();

	std::optional<winrt::Windows::ApplicationModel::StartupTaskState> GetState() const;
	void Enable();
	void Disable();

	inline bool WaitForTask() noexcept { return m_TaskAcquiredEvent.wait(); }
};
