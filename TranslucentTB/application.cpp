#include "application.hpp"
#include <WinBase.h>
#include <WinUser.h>
#include "winrt.hpp"
#include <winrt/TranslucentTB.Xaml.Pages.h>

#include "../ProgramLog/error/std.hpp"
#include "../ProgramLog/error/win32.hpp"
#include "uwp/uwp.hpp"

void Application::ConfigurationChanged(void *context, const Config &cfg)
{
	const auto that = static_cast<Application *>(context);
	that->m_Worker.ConfigurationChanged();
	that->m_AppWindow.UpdateTrayVisibility(!cfg.HideTray);
}

#ifndef DO_NOT_USE_GAME_SDK
std::unique_ptr<discord::Core> Application::CreateDiscordCore()
{
	discord::Core *core{};
	// https://github.com/discord/gamesdk-and-dispatch/issues/46
	// TODO: use a define
	const auto result = discord::Core::Create(698033470781521940, static_cast<uint64_t>(discord::CreateFlags::NoRequireDiscord), &core);
	if (core)
	{
		return std::unique_ptr<discord::Core>{ core };
	}
	else
	{
		// todo: log
		return nullptr;
	}
}

void Application::RunDiscordCallbacks()
{
	if (m_DiscordCore)
	{
		const auto result = m_DiscordCore->RunCallbacks();
		if (result != discord::Result::Ok)
		{
			m_DiscordCore = nullptr;
			// todo: log
		}
	}
}
#endif

void Application::CreateWelcomePage(bool hasPackageIdentity)
{
	using winrt::TranslucentTB::Xaml::Pages::WelcomePage;
	CreateXamlWindow<WelcomePage>(
		xaml_startup_position::center,
		[this, hasPackageIdentity](WelcomePage content)
		{
			auto closeRevoker = content.Closed(winrt::auto_revoke, [this]
			{
				// Delete the config file to redo the first start next time.
				std::error_code errc;
				// getting the config path *should* be thread-safe: nobody ever modifies it in the program lifetime
				// and Application doesn't vanish from under our feet.
				std::filesystem::remove(m_Config.GetConfigPath(), errc);
				StdErrorCodeVerify(errc, spdlog::level::warn, L"Failed to delete config file");

				m_DispatcherController.DispatcherQueue().TryEnqueue([this]
				{
					Shutdown(1);
				});
			});

			content.LiberapayOpenRequested(OpenDonationPage);

			content.DiscordJoinRequested([this]
			{
				m_DispatcherController.DispatcherQueue().TryEnqueue([this]
				{
					OpenDiscordServer();
				});
			});

			content.ConfigEditRequested([this]
			{
				m_DispatcherController.DispatcherQueue().TryEnqueue([this]
				{
					EditConfigFile();
				});
			});

			content.LicenseApproved([this, hasPackageIdentity, revoker = std::move(closeRevoker)](bool startupState) mutable
			{
				// remove the close handler because returning from the lambda will make the close event fire.
				revoker.revoke();

				m_DispatcherController.DispatcherQueue().TryEnqueue([this, hasPackageIdentity, startupState]
				{
					// This is in a different method because the lambda is freed after the first suspension point,
					// leading to use-after-free issues. An independent method doesn't have that issue, because
					// everything is stored in the coroutine frame, which is only freed once the coroutine ends.
					LicenseApprovedCallback(hasPackageIdentity, startupState);
				});
			});
		},
		hasPackageIdentity);
}

winrt::fire_and_forget Application::LicenseApprovedCallback(bool hasPackageIdentity, bool startupState)
{
	if (hasPackageIdentity && co_await m_Startup.AcquireTask())
	{
		if (startupState)
		{
			co_await m_Startup.Enable();
		}
		else
		{
			m_Startup.Disable();
		}
	}

	m_Config.SaveConfig(); // create the config file, if not already present
	m_AppWindow.RemoveHideTrayIconOverride();
}

Application::Application(HINSTANCE hInst, bool hasPackageIdentity, bool fileExists) :
	m_Config(hasPackageIdentity, fileExists, ConfigurationChanged, this),
	m_Worker(m_Config.GetConfig(), hInst),
	m_AppWindow(*this, !fileExists, !hasPackageIdentity, hInst),
	m_DispatcherController(UWP::CreateDispatcherController()),
	m_Xaml(hInst)
{
	if (const auto spam = DynamicLoader::SetPreferredAppMode())
	{
		spam(PreferredAppMode::AllowDark);
	}

	if (!fileExists)
	{
		CreateWelcomePage(hasPackageIdentity);
	}
	else if (hasPackageIdentity)
	{
		m_Startup.AcquireTask();
	}
}

void Application::OpenDonationPage()
{
	UWP::OpenUri(winrt::Windows::Foundation::Uri(L"https://liberapay.com/" APP_NAME));
}

void Application::OpenTipsPage()
{
	UWP::OpenUri(winrt::Windows::Foundation::Uri(L"https://" APP_NAME ".github.io/tips"));
}

void Application::OpenDiscordServer()
{
#ifndef DO_NOT_USE_GAME_SDK
	if (!m_DiscordCore)
	{
		m_DiscordCore = CreateDiscordCore();
	}

	if (m_DiscordCore)
	{
		m_DiscordCore->OverlayManager().OpenGuildInvite(UTF8_APP_NAME, [](discord::Result result)
		{
			if (result != discord::Result::Ok)
			{
				// todo: log and fallback
			}
		});
	}
	else
	{
		// todo: also fallback
	}
#else
	UWP::OpenUri(winrt::Windows::Foundation::Uri(L"https://discord.gg/" APP_NAME));
#endif
}

void Application::EditConfigFile()
{
	m_Config.SaveConfig();
	HresultVerify(win32::EditFile(m_Config.GetConfigPath()), spdlog::level::err, L"Failed to open configuration file.");
}

int Application::Run()
{
	while (true)
	{
		switch (MsgWaitForMultipleObjectsEx(0, nullptr, INFINITE, QS_ALLINPUT, MWMO_ALERTABLE | MWMO_INPUTAVAILABLE))
		{
		case WAIT_OBJECT_0:
			RunDiscordCallbacks();

			for (MSG msg; PeekMessage(&msg, 0, 0, 0, PM_REMOVE);)
			{
				if (msg.message != WM_QUIT)
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
				else
				{
					return static_cast<int>(msg.wParam);
				}
			}
			[[fallthrough]];
		case WAIT_IO_COMPLETION:
			continue;

		case WAIT_FAILED:
			LastErrorHandle(spdlog::level::critical, L"Failed to enter alertable wait state!");

		default:
			MessagePrint(spdlog::level::critical, L"MsgWaitForMultipleObjectsEx returned an unexpected value!");
		}
	}
}

winrt::fire_and_forget Application::Shutdown(int exitCode)
{
	// todo: close all xaml
	co_await m_DispatcherController.ShutdownQueueAsync();
	PostQuitMessage(exitCode);
}
