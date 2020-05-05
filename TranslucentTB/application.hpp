#pragma once
#include "arch.h"
#include <discord-game-sdk/core.h>
#include <memory>
#include <optional>
#include <utility>
#include <windef.h>
#include "winrt.hpp"
#include <winrt/TranslucentTB.Xaml.h>
#include "undefgetcurrenttime.h"
#include <winrt/Windows.UI.Xaml.Hosting.h>
#include "redefgetcurrenttime.h"

#include "configmanager.hpp"
#include "mainappwindow.hpp"
#include "startupmanager.hpp"
#include "taskbar/taskbarattributeworker.hpp"
#include "../ProgramLog/error/win32.hpp"
#include "uwp/xamlpagehost.hpp"

class Application final {
	static std::unique_ptr<discord::Core> CreateDiscordCore();
	static void ConfigurationChanged(void *context, const Config &cfg);

	HINSTANCE m_hInstance;

	ConfigManager m_Config;
	std::optional<StartupManager> m_Startup;

	winrt::TranslucentTB::Xaml::App m_XamlApp;
	std::vector<winrt::weak_ref<winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource>> m_XamlSources;

	std::unique_ptr<discord::Core> m_DiscordCore;

	std::optional<TaskbarAttributeWorker> m_Worker;
	std::optional<MainAppWindow> m_AppWindow;

	bool m_CompletedFirstStart;

	bool PreTranslateMessage(const MSG &msg);
	void SetupMainApplication(bool hasPackageIdentity, bool hideIconOverride);

public:
	Application(HINSTANCE hInst, bool hasPackageIdentity);

	WPARAM Run();
	void OpenDonationPage();
	void OpenDiscordServer();
	void EditConfigFile();
	void OpenTipsPage();

	inline constexpr ConfigManager &GetConfigManager() noexcept { return m_Config; }
	inline constexpr std::optional<StartupManager> &GetStartupManager() noexcept { return m_Startup; }
	inline constexpr TaskbarAttributeWorker &GetWorker() noexcept { return *m_Worker; }

	template<typename T, typename... Args>
	inline XamlPageHost<T> *CreateXamlWindow(Args&&... args)
	{
		try
		{
			// Load XAML
			if (!m_XamlApp)
			{
				m_XamlApp = { };
			}

			auto page = std::make_unique<XamlPageHost<T>>(m_hInstance, std::forward<Args>(args)...);
			m_XamlSources.push_back(page->source());
			return page.release();
		}
		HresultErrorCatch(spdlog::level::critical, L"Failed to open window");
	}

	~Application();
};
