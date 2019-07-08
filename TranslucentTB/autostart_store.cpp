#include "autostart.hpp"
#include "ttberror.hpp"
#include "ttblog.hpp"
#include "uwp.hpp"

winrt::Windows::Foundation::IAsyncOperation<Autostart::StartupState> Autostart::GetStartupState()
{
	try
	{
		co_return (co_await UWP::GetApplicationStartupTask()).State();
	}
	catch (const winrt::hresult_error &error)
	{
		ErrorHandle(error.code(), Error::Level::Log, L"Getting startup task state failed.");
		co_return StartupState::Disabled;
	}
}

winrt::Windows::Foundation::IAsyncAction Autostart::SetStartupState(StartupState state)
{
	try
	{
		const auto task = co_await UWP::GetApplicationStartupTask();
		if (state == StartupState::Enabled)
		{
			const Autostart::StartupState new_state = co_await task.RequestEnableAsync();
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
	catch (const winrt::hresult_error &error)
	{
		ErrorHandle(error.code(), Error::Level::Error, L"Changing startup task state failed!");
	}
}