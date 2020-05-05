#include "application.hpp"
#include <WinBase.h>
#include <WinUser.h>
#include <winrt/TranslucentTB.Xaml.Pages.h>

#include "uwp.hpp"

std::unique_ptr<discord::Core> Application::CreateDiscordCore()
{
	discord::Core *coreRaw{};
	// https://github.com/discord/gamesdk-and-dispatch/issues/46
	const auto result = discord::Core::Create(698033470781521940, static_cast<uint64_t>(discord::CreateFlags::NoRequireDiscord), &coreRaw);
	if (!core)
	{
		// todo: log
	}

	std::unique_ptr<discord::Core> core(coreRaw);
	return core;
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
			if (const auto native = xamlSource.try_as<IDesktopWindowXamlSourceNative2>())
			{
				BOOL result { };
				const HRESULT err = native->PreTranslateMessage(&msg, &result);
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

	if (hasPackageIdentity && !m_Startup)
	{
		m_Startup.emplace();
	}

	m_AppWindow.emplace(*this, hideIconOverride, m_hInstance);
}

Application::Application(HINSTANCE hInst, bool hasPackageIdentity) : m_hInstance(hInst), m_Config(hasPackageIdentity, ConfigurationChanged, this), m_XamlApp(nullptr), m_CompletedFirstStart(false)
{
	const bool isFirstBoot = !std::filesystem::exists(m_Config.GetConfigPath());
	SetupMainApplication(hasPackageIdentity, isFirstBoot);

	if (isFirstBoot)
	{
		const auto window = CreateXamlWindow<winrt::TranslucentTB::Xaml::Pages::WelcomePage>(hasPackageIdentity);
		const auto &content = window->content();

		content.LiberapayOpenRequested({ this, &Application::OpenDonationPage });
		content.DiscordJoinRequested({ this, &Application::OpenDiscordServer });
		content.ConfigEditRequested({ this, &Application::EditConfigFile });

		content.LicenseApproved([this, hasPackageIdentity, window](const auto &, bool startupState)
		{
			if (hasPackageIdentity)
			{
				auto &manager = m_Startup.emplace();
				if (manager.WaitForTask())
				{
					if (startupState)
					{
						manager.Enable();
					}
					else
					{
						manager.Disable();
					}
				}
				else
				{
					LastErrorHandle(spdlog::level::err, L"Failed to wait for startup task acquisition.");
				}
			}

			m_AppWindow->RemoveHideTrayIconOverride();
			m_CompletedFirstStart = true;

			if (!DestroyWindow(window->handle()))
			{
				LastErrorHandle(spdlog::level::err, L"Failed to close window???");
			}
		});

		/* TODO: what if user manually closes window? can we prevent that?
		content.LicenseDisapproved([]
		{
			PostQuitMessage(1);
		});*/
	}
	else
	{
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
	m_Config.Save();
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
