#pragma once
#include "arch.h"
#include <discord-game-sdk/core.h>
#include <memory>
#include <optional>
#include <utility>
#include <windef.h>
#include "winrt.hpp"
#include <winrt/TranslucentTB.Xaml.h>
#include <winrt/Windows.UI.Xaml.Hosting.h>

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

	bool PreTranslateMessage(const MSG &msg);
	void SetupMainApplication(bool hasPackageIdentity);

public:
	Application(HINSTANCE hInst, bool hasPackageIdentity);

	WPARAM Run();
	void OpenDiscordServer();
	void EditConfigFile();

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

			const auto page = new XamlPageHost<T>(m_hInstance, std::forward<Args>(args)...);
			m_XamlSources.push_back(page->source());
			return page;
		}
		HresultErrorCatch(spdlog::level::critical, L"Failed to open window");
	}
};
