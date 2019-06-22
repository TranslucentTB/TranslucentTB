#include "autostart.hpp"
#include "../ttberror.hpp"
#include "../ttblog.hpp"

using namespace winrt;
using namespace winrt::Windows::ApplicationModel;
using namespace winrt::Windows::Foundation;

IAsyncOperation<StartupTask> Autostart::GetApplicationStartupTask()
{
	static const auto task = (co_await StartupTask::GetForCurrentPackageAsync()).GetAt(0);
	co_return task;
}

IAsyncOperation<StartupTaskState> Autostart::GetStartupState()
{
	try
	{
		co_return (co_await GetApplicationStartupTask()).State();
	}
	catch (const hresult_error &error)
	{
		WinrtExceptionHandle(error, Error::Level::Log, L"Getting startup task state failed.");
		co_return StartupTaskState::Disabled;
	}
}

IAsyncAction Autostart::SetStartupState(StartupTaskState state)
{
	try
	{
		const auto task = co_await GetApplicationStartupTask();
		if (state == StartupTaskState::Enabled)
		{
			const auto new_state = co_await task.RequestEnableAsync();
			if (new_state != state)
			{
				Log::OutputMessage(L"Failed to change startup state.");
			}
		}
		else if (state == StartupTaskState::Disabled)
		{
			task.Disable();
		}
		else
		{
			throw std::invalid_argument("Can only set state to enabled or disabled");
		}
	}
	WinrtExceptionCatch(Error::Level::Error, L"Changing startup task state failed!")
}