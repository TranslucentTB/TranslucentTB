#include "startupmanager.hpp"
#include <winrt/Windows.Foundation.Collections.h>

#include "../../ProgramLog/error/winrt.hpp"
#include "../uwp/uwp.hpp"

winrt::fire_and_forget StartupManager::AcquireTask() try
{
	if (!m_StartupTask)
	{
		m_StartupTask = co_await winrt::Windows::ApplicationModel::StartupTask::GetAsync(APP_NAME);
	}
}
HresultErrorCatch(spdlog::level::err, L"Failed to load startup task.");

std::optional<winrt::Windows::ApplicationModel::StartupTaskState> StartupManager::GetState() const try
{
	return m_StartupTask ? std::optional(m_StartupTask.State()) : std::nullopt;
}
HresultErrorCatch(spdlog::level::warn, L"Failed to get startup task status.");

wf::IAsyncAction StartupManager::Enable() try
{
	if (m_StartupTask)
	{
		const auto result = co_await m_StartupTask.RequestEnableAsync();

		using enum winrt::Windows::ApplicationModel::StartupTaskState;
		if (result != Enabled && result != EnabledByPolicy)
		{
			MessagePrint(spdlog::level::err, APP_NAME L" asked Windows to enable a startup task but Windows did not enable it. This is typically the result of running \"privacy\" scripts on your computer.");
		}
	}
}
HresultErrorCatch(spdlog::level::err, L"Failed to enable startup task.");

void StartupManager::Disable() try
{
	if (m_StartupTask)
	{
		m_StartupTask.Disable();
	}
}
HresultErrorCatch(spdlog::level::err, L"Failed to disable startup task.");

void StartupManager::OpenSettingsPage() try
{
	UWP::OpenUri(wf::Uri(L"ms-settings:startupapps"));
}
HresultErrorCatch(spdlog::level::err, L"Failed to open Settings app.");
