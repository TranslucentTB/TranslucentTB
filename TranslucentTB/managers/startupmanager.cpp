#include "startupmanager.hpp"
#include <winrt/Windows.Foundation.Collections.h>

#include "../../ProgramLog/error/winrt.hpp"
#include "../uwp/uwp.hpp"

wf::IAsyncOperation<bool> StartupManager::AcquireTask() try
{
	if (!m_StartupTask)
	{
		m_StartupTask = co_await winrt::Windows::ApplicationModel::StartupTask::GetAsync(APP_NAME);
	}

	co_return true;
}
catch (const winrt::hresult_error &err)
{
	HresultErrorHandle(err, spdlog::level::err, L"Failed to load package startup task.");
	co_return false;
}

std::optional<winrt::Windows::ApplicationModel::StartupTaskState> StartupManager::GetState() const
{
	if (m_StartupTask)
	{
		try
		{
			return m_StartupTask.State();
		}
		HresultErrorCatch(spdlog::level::warn, L"Failed to get startup task status.");
	}

	return std::nullopt;
}

wf::IAsyncAction StartupManager::Enable()
{
	if (m_StartupTask)
	{
		using winrt::Windows::ApplicationModel::StartupTaskState;

		StartupTaskState result;
		try
		{
			result = co_await m_StartupTask.RequestEnableAsync();
		}
		catch (const winrt::hresult_error &err)
		{
			HresultErrorHandle(err, spdlog::level::err, L"Failed to enable startup task.");
			co_return;
		}

		if (result != StartupTaskState::Enabled && result != StartupTaskState::EnabledByPolicy)
		{
			MessagePrint(spdlog::level::err, APP_NAME L" asked Windows to enable a startup task but Windows did not enable it. This is typically the result of running \"privacy\" scripts on your computer.");
		}
	}
}

void StartupManager::Disable()
{
	if (m_StartupTask)
	{
		try
		{
			m_StartupTask.Disable();
		}
		HresultErrorCatch(spdlog::level::err, L"Failed to disable startup task.");
	}
}

void StartupManager::OpenSettingsPage()
{
	UWP::OpenUri(wf::Uri(L"ms-settings:startupapps"));
}
