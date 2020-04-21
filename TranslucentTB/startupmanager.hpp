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

	wil::srwlock m_TaskLock;
	winrt::Windows::ApplicationModel::StartupTask m_StartupTask;

public:
	template<typename T>
	StartupManager(T &&callback) try : m_StartupTask(nullptr)
	{
		using namespace winrt::Windows::ApplicationModel;
		using namespace winrt::Windows::Foundation;
		using Collections::IVectorView;

		StartupTask::GetForCurrentPackageAsync().Completed([this, call = std::forward<T>(callback)] (const IAsyncOperation<IVectorView<StartupTask>> &op, AsyncStatus)
		{
			try
			{
				auto result = op.GetResults().GetAt(0);

				const auto lock = m_TaskLock.lock_exclusive();
				m_StartupTask = std::move(result);
				call();
			}
			HresultErrorCatch(spdlog::level::critical, L"Failed to get first startup task.");
		});
	}
	HresultErrorCatch(spdlog::level::critical, L"Failed to load package startup tasks.");

	// allow empty callback by default construction
	StartupManager() : StartupManager([] { }) { }

	std::optional<winrt::Windows::ApplicationModel::StartupTaskState> GetState();
	void Enable();
	void Disable();
};
