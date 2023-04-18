#include "application.hpp"
#include <WinBase.h>
#include <WinUser.h>
#include "winrt.hpp"
#include <winrt/Windows.System.h>
#include <winrt/TranslucentTB.Xaml.Pages.h>
#include <wil/cppwinrt_helpers.h>

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

void Application::CreateWelcomePage()
{
	using winrt::TranslucentTB::Xaml::Pages::WelcomePage;
	CreateXamlWindow<WelcomePage>(
		xaml_startup_position::center,
		[this](const WelcomePage &content, BaseXamlPageHost *host)
		{
			DispatchToMainThread([this, hwnd = host->handle()]
			{
				m_WelcomePage = hwnd;
			});

			auto closeRevoker = content.Closed(winrt::auto_revoke, [this]
			{
				DispatchToMainThread([this]
				{
					Shutdown();
				});
			});

			content.LiberapayOpenRequested(OpenDonationPage);
			content.DiscordJoinRequested(OpenDiscordServer);

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
					m_WelcomePage = nullptr;
					m_Config.SaveConfig(); // create the config file, if not already present
					m_AppWindow.RemoveHideTrayIconOverride();
					m_AppWindow.SendNotification(IDS_WELCOME_NOTIFICATION);
				});
			});
		});
}

Application::Application(HINSTANCE hInst, std::optional<std::filesystem::path> storageFolder, bool fileExists) :
	m_Config(storageFolder, fileExists, ConfigurationChanged, this),
	m_DispatcherController(UWP::CreateDispatcherController()),
	m_Worker(m_Config.GetConfig(), hInst, m_Loader, storageFolder),
	m_UwpCRTDep(
		hInst,
		L"Microsoft.VCLibs.140.00_8wekyb3d8bbwe",
		PACKAGE_VERSION {
			// 14.0.30704.0 but the order is reversed because that's how the struct is.
			.Revision = 0,
			.Build = 30704,
			.Minor = 0,
			.Major = 14
		},
		storageFolder.has_value()
	),
	m_WinUIDep(
		hInst,
		L"Microsoft.UI.Xaml.2.8_8wekyb3d8bbwe",
		PACKAGE_VERSION {
			// 8.2304.12003.0 but the order is reversed because that's how the struct is.
			.Revision = 0,
			.Build = 12003,
			.Minor = 2304,
			.Major = 8
		},
		storageFolder.has_value()
	),
	m_XamlApp(CreateXamlApp()),
	m_XamlManager(UWP::CreateXamlManager()),
	m_AppWindow(*this, !fileExists, storageFolder.has_value(), hInst, m_Loader),
	m_Xaml(hInst),
	m_ShuttingDown(false)
{
	if (const auto spam = m_Loader.SetPreferredAppMode())
	{
		spam(PreferredAppMode::AllowDark);
	}

	if (storageFolder)
	{
		m_Startup.AcquireTask();
	}

	if (!fileExists)
	{
		CreateWelcomePage();
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
	UWP::OpenUri(wf::Uri(L"https://discord.gg/" APP_NAME));
}

int Application::Run()
{
	while (true)
	{
		switch (MsgWaitForMultipleObjectsEx(0, nullptr, INFINITE, QS_ALLINPUT, MWMO_ALERTABLE | MWMO_INPUTAVAILABLE))
		{
		case WAIT_OBJECT_0:
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

winrt::fire_and_forget Application::Shutdown()
{
	// several calls to Shutdown crash the app because DispatcherController::ShutdownQueueAsync
	// can only be called once. but it can happen that Shutdown is called several times
	// e.g. the user uninstalls the app while the welcome page is opened, which causes Shutdown
	// to close the welcome page, which tries to shutdown the app.
	if (!m_ShuttingDown)
	{
		m_ShuttingDown = true;
		const bool hasWelcomePage = m_WelcomePage != nullptr;

		bool canExit = true;
		for (const auto &thread : m_Xaml.GetThreads())
		{
			const auto guard = thread->Lock();
			if (const auto &window = thread->GetCurrentWindow(); window && window->page())
			{
				// Checking if the window can be closed requires to be on the same thread, so
				// switch to that thread.
				co_await wil::resume_foreground(thread->GetDispatcher());
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
			co_await wil::resume_foreground(m_DispatcherController.DispatcherQueue());

			co_await m_DispatcherController.ShutdownQueueAsync();

			// delete the config file if the user hasn't went through the welcome page
			// to make them go through it again next startup.
			if (hasWelcomePage)
			{
				m_Config.DeleteConfigFile();
			}

			// exit
			PostQuitMessage(hasWelcomePage ? 1 : 0);
		}
	}
}

bool Application::BringWelcomeToFront() noexcept
{
	if (m_WelcomePage)
	{
		SetForegroundWindow(m_WelcomePage);
		return true;
	}
	else
	{
		return false;
	}
}
