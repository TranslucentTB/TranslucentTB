#include "application.hpp"
#include <DispatcherQueue.h>
#include <WinBase.h>
#include <WinUser.h>
#include <winrt/TranslucentTB.Xaml.Pages.h>

#include "uwp/uwp.hpp"

winrt::Windows::System::DispatcherQueueController Application::CreateDispatcher()
{
	const DispatcherQueueOptions options = {
		.dwSize = sizeof(options),
		.threadType = DQTYPE_THREAD_CURRENT,
		.apartmentType = DQTAT_COM_STA
	};

	ABI::Windows::System::IDispatcherQueueController *controller;
	HresultVerify(CreateDispatcherQueueController(options, &controller), spdlog::level::critical, L"Failed to create dispatcher!");
	return { controller, winrt::take_ownership_from_abi };
}

void Application::ConfigurationChanged(void *context, const Config &cfg)
{
	const auto that = static_cast<Application *>(context);
	that->m_Worker.ConfigurationChanged();
	that->m_AppWindow.UpdateTrayVisibility(!cfg.HideTray);
}

#ifndef DO_NOT_USE_GAME_SDK
std::unique_ptr<discord::Core> Application::CreateDiscordCore()
{
	discord::Core *coreRaw{};
	// https://github.com/discord/gamesdk-and-dispatch/issues/46
	const auto result = discord::Core::Create(698033470781521940, static_cast<uint64_t>(discord::CreateFlags::NoRequireDiscord), &coreRaw);
	if (coreRaw)
	{
		std::unique_ptr<discord::Core> core(coreRaw);
		return core;
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
	CreateXamlWindow<WelcomePage>([this](XamlPageHost<WelcomePage> &host)
	{
		const auto &content = host.content();

		content.LiberapayOpenRequested({ this, &Application::OpenDonationPage });

#ifndef DO_NOT_USE_GAME_SDK
		content.DiscordJoinRequested([this]() -> winrt::fire_and_forget
		{
			co_await m_Dispatcher.DispatcherQueue();
			// TODO: this behaves weirdly
			OpenDiscordServer();
		});
#else
		content.DiscordJoinRequested({ this, &Application::OpenDiscordServer });
#endif

		content.ConfigEditRequested([this]() -> winrt::fire_and_forget
		{
			co_await m_Dispatcher.DispatcherQueue();
			EditConfigFile();
		});

		content.LicenseApproved([this](bool startupState) -> winrt::fire_and_forget
		{
			// set this first because awaiting will make the callback return,
			// causing the Closed event to fire and call PostQuitMessage.
			m_CompletedFirstStart = true;

			// move back to the main thread.
			// it's not safe to co_await in the calling thread because as
			// soon as we hit the first suspension point, the window and
			// message loop is torn down.
			co_await m_Dispatcher.DispatcherQueue();
			m_AppWindow->RemoveHideTrayIconOverride();

			if (m_Startup && co_await m_Startup->AcquireTask())
			{
				if (startupState)
				{
					co_await m_Startup->Enable();
				}
				else
				{
					m_Startup->Disable();
				}
			}
		});

		content.Closed([this]() -> winrt::fire_and_forget
		{
			if (!m_CompletedFirstStart)
			{
				co_await m_Dispatcher.DispatcherQueue();
				PostQuitMessage(1);
			}
		});
	}, hasPackageIdentity);
}

Application::Application(HINSTANCE hInst, bool hasPackageIdentity, bool fileExists) :
	m_hInstance(hInst),
	m_Dispatcher(CreateDispatcher()),
	m_Config(hasPackageIdentity, fileExists, ConfigurationChanged, this),
	m_Worker(m_Config.GetConfig(), m_hInstance),
	m_AppWindow(*this, !fileExists, !hasPackageIdentity, hInst),
	m_XamlApp(nullptr),
	m_CompletedFirstStart(fileExists)
{
	if (!fileExists)
	{
		CreateWelcomePage(hasPackageIdentity);
	}
	else
	{
		if (hasPackageIdentity)
		{
			m_Startup.AcquireTask();
		}
	}
}

void Application::OpenDonationPage()
{
	UWP::OpenUri(winrt::Windows::Foundation::Uri(L"https://liberapay.com/" APP_NAME));
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

void Application::OpenTipsPage()
{
	UWP::OpenUri(winrt::Windows::Foundation::Uri(L"https://" APP_NAME ".github.io/tips"));
}

Application::~Application()
{
	if (!m_CompletedFirstStart)
	{
		// Redo the first start next time.
		std::filesystem::remove(m_Config.GetConfigPath());
	}
}
