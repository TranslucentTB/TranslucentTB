#include "application.hpp"
#include <WinBase.h>
#include <WinUser.h>
#include <winrt/TranslucentTB.Xaml.Pages.h>

#include "uwp.hpp"

std::unique_ptr<discord::Core> Application::CreateDiscordCore()
{
	discord::Core *core{};
	// https://github.com/discord/gamesdk-and-dispatch/issues/46
	const auto result = discord::Core::Create(698033470781521940, static_cast<uint64_t>(discord::CreateFlags::NoRequireDiscord), &core);
	if (!core)
	{
		// todo: log
	}

	return std::unique_ptr<discord::Core> { core };
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
				BOOL result{};
				HRESULT err = native->PreTranslateMessage(&msg, &result);
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

void Application::SetupMainApplication(bool hasPackageIdentity)
{
	TaskbarAttributeWorker &worker = m_Worker.emplace(m_Config.GetConfig(), m_hInstance);

	if (hasPackageIdentity && !m_Startup)
	{
		m_Startup.emplace();
	}

	m_AppWindow.emplace(m_Startup, m_Config, worker, m_hInstance);
}

Application::Application(HINSTANCE hInst, bool hasPackageIdentity) : m_hInstance(hInst), m_Config(hasPackageIdentity, ConfigurationChanged, this), m_XamlApp(nullptr)
{
	if (std::filesystem::exists(m_Config.GetConfigPath()))
	{
		SetupMainApplication(hasPackageIdentity);
	}
	else
	{
		const auto window = CreateXamlWindow<winrt::TranslucentTB::Xaml::Pages::WelcomePage>();
		const auto &content = window->content();
		content.DiscordJoinRequested([this]
		{
			OpenDiscordServer();
		});

		content.ConfigEditRequested([this]
		{
			// save default configuration
			m_Config.Save();

			EditConfigFile();
		});

		/*content.LicenseApproved([this, hasPackageIdentity](bool startupState)
		{
			if (hasPackageIdentity)
			{
				m_Startup.emplace([this, startupState]
				{
					if (startupState)
					{
						m_Startup.Enable();
					}
					else
					{
						m_Startup.Disable();
					}
				});
			}

			SetupMainApplication();
		});*/
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

void Application::OpenDiscordServer()
{
	if (!m_DiscordCore)
	{
		m_DiscordCore = CreateDiscordCore();
	}

	if (m_DiscordCore)
	{
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
		// fallback
	}
}

void Application::EditConfigFile()
{
	HresultVerify(win32::EditFile(m_Config.GetConfigPath()), spdlog::level::err, L"Failed to open text editor");
}
