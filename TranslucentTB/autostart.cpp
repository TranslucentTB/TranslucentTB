#define _EXPERIMENTAL_RESUMABLE_
#include "coroutine.hpp"
#define _RESUMABLE_FUNCTIONS_SUPPORTED
#include "autostart.hpp"
#include "ttberror.hpp"
#include "ttblog.hpp"

using namespace winrt::Windows::Foundation;

IAsyncOperation<winrt::Windows::ApplicationModel::StartupTask> Autostart::GetApplicationStartupTask()
{
	static const auto task = (co_await winrt::Windows::ApplicationModel::StartupTask::GetForCurrentPackageAsync()).GetAt(0);
	co_return task;
}

IAsyncOperation<Autostart::StartupState> Autostart::GetStartupState()
{
	try
	{
		co_return (co_await GetApplicationStartupTask()).State();
	}
	catch (...)
	{
		ErrorHandle(winrt::to_hresult(), Error::Level::Log, L"Getting startup task state failed.");
		co_return StartupState::Disabled;
	}
}

IAsyncAction Autostart::SetStartupState(StartupState state)
{
	try
	{
		const auto task = co_await GetApplicationStartupTask();
		if (state == StartupState::Enabled)
		{
			const auto new_state = co_await task.RequestEnableAsync();
			if (new_state != state)
			{
				Log::OutputMessage(L"Failed to change startup state.");
			}
		}
		else if (state == StartupState::Disabled)
		{
			task.Disable();
		}
		else
		{
			throw std::invalid_argument("Can only set state to enabled or disabled");
		}
	}
	catch (...)
	{
		ErrorHandle(winrt::to_hresult(), Error::Level::Error, L"Changing startup task state failed!");
	}
}