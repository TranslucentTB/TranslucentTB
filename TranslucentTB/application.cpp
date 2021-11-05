#include "application.hpp"
#include <WinBase.h>
#include <WinUser.h>
#include "winrt.hpp"
#include <winrt/TranslucentTB.Xaml.Pages.h>

#include "../ProgramLog/error/std.hpp"
#include "../ProgramLog/error/win32.hpp"
#include "uwp/uwp.hpp"

void Application::ConfigurationChanged(void *context)
{
	const auto that = static_cast<Application *>(context);
	that->m_Worker.ConfigurationChanged();
	that->m_AppWindow.ConfigurationChanged();
}

winrt::TranslucentTB::Xaml::App Application::CreateXamlApp() try
{
	return { };
}
HresultErrorCatch(spdlog::level::critical, L"Failed to create Xaml app");

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

winrt::fire_and_forget Application::CreateWelcomePage(wf::IAsyncOperation<bool> operation)
{
	// done first because the closed callback would otherwise need to await it
	// and we can only await an IAsyncOperation once.
	if (operation)
	{
		co_await operation;
		co_await m_Startup.Enable();
	}

	using winrt::TranslucentTB::Xaml::Pages::WelcomePage;
	CreateXamlWindow<WelcomePage>(
		xaml_startup_position::center,
		[this, hasStartup = operation != nullptr](const WelcomePage &content, BaseXamlPageHost *)
		{
			auto closeRevoker = content.Closed(winrt::auto_revoke, [this, hasStartup]
			{
				DispatchToMainThread([this, hasStartup]
				{
					if (hasStartup)
					{
						m_Startup.Disable();
					}

					m_Config.DeleteConfigFile();
					Shutdown(1);
				});
			});

			content.LiberapayOpenRequested(OpenDonationPage);

#ifndef DO_NOT_USE_GAME_SDK
			content.DiscordJoinRequested([this]
			{
				DispatchToMainThread([this]
				{
					OpenDiscordServer();
				});
			});
#else
			content.DiscordJoinRequested(OpenDiscordServer);
#endif

			content.ConfigEditRequested([this]
			{
				DispatchToMainThread([this]
				{
					m_Config.EditConfigFile();
				});
			});

			content.LicenseApproved([this, revoker = std::move(closeRevoker)]() mutable
			{
				// remove the close handler because returning from the lambda will make the close event fire.
				revoker.revoke();

				DispatchToMainThread([this]
				{
					m_Config.SaveConfig(); // create the config file, if not already present
					m_AppWindow.RemoveHideTrayIconOverride();
					m_AppWindow.SendNotification(IDS_WELCOME_NOTIFICATION);
				});
			});
		});
}

Application::Application(HINSTANCE hInst, std::optional<std::filesystem::path> storageFolder, bool fileExists) :
	// seemingly, dynamic deps are not transitive so add a dyn dep to the CRT that WinUI uses.
	m_UwpCRTDep(
		L"Microsoft.VCLibs.140.00_8wekyb3d8bbwe",
		PACKAGE_VERSION {
			// 14.0.30035.0 but the order is reversed because that's how the struct is.
			.Revision = 0,
			.Build = 30035,
			.Minor = 0,
			.Major = 14
		},
		storageFolder.has_value()
	),
	m_WinUIDep(
		L"Microsoft.UI.Xaml.2.7_8wekyb3d8bbwe",
		PACKAGE_VERSION {
			// 7.2109.13004.0 but the order is reversed because that's how the struct is.
			.Revision = 0,
			.Build = 13004,
			.Minor = 2109,
			.Major = 7
		},
		storageFolder.has_value()
	),
	m_Config(storageFolder, fileExists, ConfigurationChanged, this),
	m_Worker(m_Config.GetConfig(), hInst, m_Loader, storageFolder),
	m_DispatcherController(UWP::CreateDispatcherController()),
	m_XamlApp(CreateXamlApp()),
	m_XamlManager(UWP::CreateXamlManager()),
	m_AppWindow(*this, !fileExists, storageFolder.has_value(), hInst, m_Loader),
	m_Xaml(hInst)
{
	if (const auto spam = m_Loader.SetPreferredAppMode())
	{
		spam(PreferredAppMode::AllowDark);
	}

	wf::IAsyncOperation<bool> op = storageFolder ? m_Startup.AcquireTask() : nullptr;
	if (!fileExists)
	{
		CreateWelcomePage(std::move(op));
	}
}

void Application::OpenDonationPage()
{
	UWP::OpenUri(wf::Uri(L"https://liberapay.com/" APP_NAME));
}

void Application::OpenTipsPage()
{
	UWP::OpenUri(wf::Uri(L"https://" APP_NAME ".github.io/tips"));
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
	UWP::OpenUri(wf::Uri(L"https://discord.gg/" APP_NAME));
#endif
}

int Application::Run()
{
	while (true)
	{
		switch (MsgWaitForMultipleObjectsEx(0, nullptr, INFINITE, QS_ALLINPUT, MWMO_ALERTABLE | MWMO_INPUTAVAILABLE))
		{
		case WAIT_OBJECT_0:
#ifndef DO_NOT_USE_GAME_SDK
			RunDiscordCallbacks();
#endif

			for (MSG msg; PeekMessage(&msg, 0, 0, 0, PM_REMOVE);)
			{
				if (msg.message != WM_QUIT)
				{
					if (!m_AppWindow.PreTranslateMessage(msg))
					{
						TranslateMessage(&msg);
						DispatchMessage(&msg);
					}
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
	bool canExit = true;
	for (const auto &thread : m_Xaml.GetThreads())
	{
		const auto guard = thread->Lock();
		if (const auto &window = thread->GetCurrentWindow(); window && window->page())
		{
			// Checking if the window can be closed requires to be on the same thread, so
			// switch to that thread.
			co_await thread->GetDispatcher();
			if (!window->TryClose())
			{
				canExit = false;

				// bring attention to the window that couldn't be closed.
				SetForegroundWindow(window->handle());
			}
		}
	}

	if (canExit)
	{
		// go back to the main thread for exiting
		co_await m_DispatcherController.DispatcherQueue();

		co_await m_DispatcherController.ShutdownQueueAsync();

		// exit
		PostQuitMessage(exitCode);
	}
}
