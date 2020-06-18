#include "application.hpp"
#include <DispatcherQueue.h>
#include <WinBase.h>
#include <WinUser.h>
#include <winrt/TranslucentTB.Xaml.Pages.h>

#include "uwp.hpp"

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

bool Application::PreTranslateMessage(const MSG &msg)
{
	for (auto it = m_XamlSources.begin(); it != m_XamlSources.end();)
	{
		if (const auto xamlSource = it->get())
		{
			BOOL result { };
			const HRESULT err = xamlSource->PreTranslateMessage(&msg, &result);
			if (SUCCEEDED(err))
			{
				if (result)
				{
					return true;
				}
			}
			else
			{
				HresultHandle(err, spdlog::level::warn, L"Failed to pre-translate message.");
			}

			++it;
		}
		else
		{
			// remove invalid weak references
			it = m_XamlSources.erase(it);
		}
	}

	return false;
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

Application::Application(HINSTANCE hInst, bool hasPackageIdentity) : m_hInstance(hInst), m_Dispatcher(CreateDispatcher()), m_Config(hasPackageIdentity, ConfigurationChanged, this), m_XamlApp(nullptr), m_CompletedFirstStart(false)
{
	const bool isFirstBoot = !m_Config.LoadConfig();
	SetupMainApplication(hasPackageIdentity, isFirstBoot);

	if (isFirstBoot)
	{
		const auto window = CreateXamlWindow<winrt::TranslucentTB::Xaml::Pages::WelcomePage>(hasPackageIdentity);
		const auto &content = window->content();

		content.LiberapayOpenRequested({ this, &Application::OpenDonationPage });
		content.DiscordJoinRequested({ this, &Application::OpenDiscordServer });
		content.ConfigEditRequested({ this, &Application::EditConfigFile });

		content.LicenseApproved([this](bool startupState) -> winrt::fire_and_forget
		{
			// set this first because awaiting will make the callback return,
			// causing the Closed event to fire and call PostQuitMessage.
			m_CompletedFirstStart = true;

			if (m_Startup)
			{
				co_await m_Startup->AcquireTask();
				if (startupState)
				{
					co_await m_Startup->Enable();
				}
				else
				{
					m_Startup->Disable();
				}

				// return to the UI thread
				co_await winrt::resume_foreground(m_Dispatcher.DispatcherQueue());
			}

			m_AppWindow->RemoveHideTrayIconOverride();
		});

		content.Closed([this]
		{
			if (!m_CompletedFirstStart)
			{
				PostQuitMessage(1);
			}
		});

		if (!SetForegroundWindow(window->handle()))
		{
			MessagePrint(spdlog::level::warn, L"Failed to set foreground window");
		}
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

WPARAM Application::Run()
{
	while (true)
	{
		switch (MsgWaitForMultipleObjectsEx(0, nullptr, INFINITE, QS_ALLINPUT, MWMO_ALERTABLE | MWMO_INPUTAVAILABLE))
		{
		case WAIT_OBJECT_0:
			if (m_DiscordCore)
			{
				const auto result = m_DiscordCore->RunCallbacks();
				if (result != discord::Result::Ok)
				{
					m_DiscordCore = nullptr;
					// todo: log
				}
			}

			for (MSG msg; PeekMessage(&msg, 0, 0, 0, PM_REMOVE);)
			{
				if (msg.message != WM_QUIT)
				{
					if (!PreTranslateMessage(msg))
					{
						TranslateMessage(&msg);
						DispatchMessage(&msg);
					}
				}
				else
				{
					return msg.wParam;
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

Application::~Application()
{
	if (!m_CompletedFirstStart)
	{
		// Redo the first start next time.
		std::filesystem::remove(m_Config.GetConfigPath());
	}
}
