#include "application.hpp"
#include <DispatcherQueue.h>
#include <WinBase.h>
#include <WinUser.h>
#include <winrt/TranslucentTB.Xaml.Pages.h>

#include "uwp.hpp"

using winrt::Windows::Foundation::IAsyncOperation;

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

void Application::ConfigurationChanged(void *context, const Config &cfg)
{
	const auto that = static_cast<Application *>(context);
	if (that->m_Worker)
	{
		that->m_Worker->ConfigurationChanged();
	}

	if (that->m_AppWindow)
	{
		that->m_AppWindow->UpdateTrayVisibility(!cfg.HideTray);
	}
}

void Application::SetupMainApplication(bool hasPackageIdentity, bool hideIconOverride)
{
	m_Worker.emplace(m_Config.GetConfig(), m_hInstance);

	if (hasPackageIdentity)
	{
		m_Startup.emplace();
	}

	m_AppWindow.emplace(*this, hideIconOverride, m_hInstance);
}

void Application::CreateWelcomePage(bool hasPackageIdentity)
{
	using winrt::TranslucentTB::Xaml::Pages::WelcomePage;
	CreateXamlWindow<WelcomePage>([this](XamlPageHost<WelcomePage> &host)
	{
		const auto &content = host.content();

		content.LiberapayOpenRequested({ this, &Application::OpenDonationPage });

		content.DiscordJoinRequested([this]() -> winrt::fire_and_forget
		{
			co_await m_Dispatcher.DispatcherQueue();
			// TODO: this behaves weirdly
			OpenDiscordServer();
		});

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

Application::Application(HINSTANCE hInst, bool hasPackageIdentity) : m_hInstance(hInst), m_Dispatcher(CreateDispatcher()), m_Config(hasPackageIdentity, ConfigurationChanged, this), m_XamlApp(nullptr), m_CompletedFirstStart(false)
{
	const bool isFirstBoot = !m_Config.LoadConfig();
	SetupMainApplication(hasPackageIdentity, isFirstBoot);

	if (isFirstBoot)
	{
		CreateWelcomePage(hasPackageIdentity);
	}
	else
	{
		if (m_Startup)
		{
			m_Startup->AcquireTask();
		}

		m_CompletedFirstStart = true;
	}
}

void Application::OpenDonationPage()
{
	HresultVerify(win32::OpenLink(L"https://liberapay.com/" APP_NAME), spdlog::level::err, L"Failed to open Liberapay link");
}

void Application::OpenDiscordServer()
{
	if (!m_DiscordCore)
	{
		m_DiscordCore = CreateDiscordCore();
	}

	if (m_DiscordCore)
	{
		// TODO: actual invite
		m_DiscordCore->OverlayManager().OpenGuildInvite("discord-linux", [](discord::Result result)
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
}

void Application::EditConfigFile()
{
	m_Config.SaveConfig();
	HresultVerify(win32::EditFile(m_Config.GetConfigPath()), spdlog::level::err, L"Failed to open configuration file.");
}

void Application::OpenTipsPage()
{
	HresultVerify(win32::OpenLink(L"https://" APP_NAME ".github.io/tips"), spdlog::level::err, L"Failed to open tips & tricks link.");
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

Application::~Application()
{
	if (!m_CompletedFirstStart)
	{
		// Redo the first start next time.
		std::filesystem::remove(m_Config.GetConfigPath());
	}
}
