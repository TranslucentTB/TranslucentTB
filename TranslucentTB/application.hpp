#pragma once
#include "arch.h"
#include <atomic>
#include <discord-game-sdk/core.h>
#include <functional>
#include <memory>
#include <optional>
#include <thread>
#include <utility>
#include <windef.h>
#include "winrt.hpp"
#include <winrt/TranslucentTB.Xaml.h>
#include "undefgetcurrenttime.h"
#include <winrt/Windows.UI.Xaml.Hosting.h>
#include "redefgetcurrenttime.h"
#include <windows.ui.xaml.hosting.desktopwindowxamlsource.h>
#include <winrt/Windows.System.h>

#include "configmanager.hpp"
#include "mainappwindow.hpp"
#include "startupmanager.hpp"
#include "taskbar/taskbarattributeworker.hpp"
#include "../ProgramLog/error/winrt.hpp"
#include "uwp/xamlpagehost.hpp"

class Application final {
	static winrt::Windows::System::DispatcherQueueController CreateDispatcher();
	static std::unique_ptr<discord::Core> CreateDiscordCore();
	static void ConfigurationChanged(void *context, const Config &cfg);

	HINSTANCE m_hInstance;

	winrt::Windows::System::DispatcherQueueController m_Dispatcher;

	ConfigManager m_Config;
	std::optional<TaskbarAttributeWorker> m_Worker;
	std::optional<StartupManager> m_Startup;
	std::optional<MainAppWindow> m_AppWindow;

	winrt::TranslucentTB::Xaml::App m_XamlApp;
	std::unique_ptr<discord::Core> m_DiscordCore;

	std::atomic<bool> m_CompletedFirstStart;

	void SetupMainApplication(bool hasPackageIdentity, bool hideIconOverride);
	void CreateWelcomePage(bool hasPackageIdentity);

public:
	Application(HINSTANCE hInst, bool hasPackageIdentity);

	void OpenDonationPage();
	void OpenDiscordServer();
	void EditConfigFile();
	void OpenTipsPage();

	inline constexpr ConfigManager &GetConfigManager() noexcept { return m_Config; }
	inline constexpr std::optional<StartupManager> &GetStartupManager() noexcept { return m_Startup; }
	inline constexpr TaskbarAttributeWorker &GetWorker() noexcept { return *m_Worker; }

	void RunDiscordCallbacks();

	inline WPARAM Run()
	{
		return m_AppWindow->Run();
	}

	template<typename T, typename Callback, typename... Args>
	inline void CreateXamlWindow(Callback &&callback, Args... args)
	{
		// Load XAML
		if (!m_XamlApp)
		{
			try
			{
				m_XamlApp = { };
			}
			HresultErrorCatch(spdlog::level::critical, L"Failed to load XAML resources");
		}

		std::thread([hInst = m_hInstance, callback = std::forward<Callback>(callback), ...args = std::move(args)]
		{
			try
			{
				XamlPageHost<T> page(hInst, std::move(args)...);
				std::invoke(callback, page);
				page.Run();
			}
			HresultErrorCatch(spdlog::level::critical, L"Failed to create window");
		}).detach();
	}

	~Application();
};
