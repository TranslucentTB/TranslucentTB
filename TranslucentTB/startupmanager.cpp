#include "startupmanager.hpp"
#include <winrt/Windows.Foundation.Collections.h>

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::ApplicationModel;

StartupTask StartupManager::GetTaskSafe() const noexcept
{
	const auto lock = m_TaskLock.acquire();
	return m_StartupTask;
}

IAsyncOperation<bool> StartupManager::AcquireTask() try
{
	const auto lock = m_TaskLock.acquire();

	if (!m_StartupTask)
	{
		m_StartupTask = (co_await StartupTask::GetForCurrentPackageAsync()).GetAt(0);
	}

	co_return true;
}
catch (const winrt::hresult_error &err)
{
	HresultErrorHandle(err, spdlog::level::err, L"Failed to load package startup task.");
	co_return false;
}

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
		catch (const winrt::hresult_error &err)
		{
			HresultErrorHandle(err, spdlog::level::err, L"Failed to enable startup task.");
			co_return;
		}

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
