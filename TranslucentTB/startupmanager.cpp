#include "startupmanager.hpp"

using namespace winrt::Windows::Foundation;
using winrt::Windows::ApplicationModel::StartupTaskState;

std::optional<StartupTaskState> StartupManager::GetState()
{
	try
	{
		const auto lock = m_TaskLock.lock_shared();
		if (m_StartupTask)
		{
			return m_StartupTask.State();
		}
	}
	catch (const winrt::hresult_error &err)
	{
		HresultErrorHandle(err, spdlog::level::warn, L"Failed to get startup task status.");
	}

	return std::nullopt;
}

void StartupManager::Enable() try
{
	const auto lock = m_TaskLock.lock_shared();
	m_StartupTask.RequestEnableAsync().Completed([](const IAsyncOperation<StartupTaskState> &op, AsyncStatus)
	{
		try
		{
			const auto result = op.GetResults();
			if (result != StartupTaskState::Enabled &&
				result != StartupTaskState::EnabledByPolicy)
			{
				MessagePrint(spdlog::level::err, L"A request to enable the startup task did not result in it being enabled.");
			}
		}
		HresultErrorCatch(spdlog::level::err, FAILED_TO_ENABLE);
	});
}
HresultErrorCatch(spdlog::level::err, FAILED_TO_ENABLE);

void StartupManager::Disable() try
{
	const auto lock = m_TaskLock.lock_shared();
	m_StartupTask.Disable();
}
HresultErrorCatch(spdlog::level::err, L"Failed to disable startup task.");
