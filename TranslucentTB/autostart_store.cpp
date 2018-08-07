#include "autostart.hpp"
#include "ttberror.hpp"
#include "ttblog.hpp"
#include "uwp.hpp"

concurrency::task<Autostart::StartupState> Autostart::GetStartupState()
{
	return concurrency::create_task([]() -> StartupState
	{
		try
		{
			return UWP::GetApplicationStartupTask().get()->State();
		}
		catch (const winrt::hresult_error &error)
		{
			ErrorHandle(error.code(), Error::Level::Log, L"Getting startup task state failed.");
			return StartupState::Disabled;
		}
	});
}

concurrency::task<void> Autostart::SetStartupState(const StartupState &state)
{
	return concurrency::create_task([=]
	{
		try
		{
			const auto task = *UWP::GetApplicationStartupTask().get();
			if (state == StartupState::Enabled)
			{
				const Autostart::StartupState new_state = task.RequestEnableAsync().get();
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
	});
}