#include "startupmanager.hpp"
#include <winrt/Windows.Foundation.Collections.h>

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::ApplicationModel;

StartupTask StartupManager::GetTaskSafe() const noexcept
{
	const auto lock = m_TaskLock.lock_shared();
	return m_StartupTask;
}

IAsyncAction StartupManager::AcquireTask() try
{
	const auto lock = m_TaskLock.lock_exclusive();

	if (!m_StartupTask)
	{
		const auto task = (co_await StartupTask::GetForCurrentPackageAsync()).GetAt(0);

		m_StartupTask = std::move(task);
	}
}
HresultErrorCatch(spdlog::level::critical, L"Failed to load package startup task.");

std::optional<StartupTaskState> StartupManager::GetState() const
{
	if (const auto task = GetTaskSafe())
	{
		try
		{
			return task.State();
		}
		HresultErrorCatch(spdlog::level::warn, L"Failed to get startup task status.");
	}

	return std::nullopt;
}

IAsyncAction StartupManager::Enable()
{
	if (const auto task = GetTaskSafe())
	{
		StartupTaskState result;
		try
		{
			result = co_await task.RequestEnableAsync();
		}
		HresultErrorCatch(spdlog::level::err, L"Failed to enable startup task.");

		if (result != StartupTaskState::Enabled && result != StartupTaskState::EnabledByPolicy)
		{
			MessagePrint(spdlog::level::err, L"A request to enable the startup task did not result in it being enabled.");
		}
	}
}

void StartupManager::Disable()
{
	if (const auto task = GetTaskSafe())
	{
		try
		{
			task.Disable();
		}
		HresultErrorCatch(spdlog::level::err, L"Failed to disable startup task.");
	}
}
