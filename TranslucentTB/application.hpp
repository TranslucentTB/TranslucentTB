#pragma once
#include "arch.h"
#include <memory>
#include <windef.h>
#include "winrt.hpp"
#include <winrt/Windows.System.h>

#include "managers/configmanager.hpp"
#include "mainappwindow.hpp"
#include "managers/startupmanager.hpp"
#include "taskbar/taskbarattributeworker.hpp"
#include "uwp/xamlcontentmanager.hpp"

#ifndef DO_NOT_USE_GAME_SDK
#include <discord-game-sdk/core.h>
#endif

class Application final {
	static winrt::Windows::System::DispatcherQueueController CreateDispatcherController();
	static void ConfigurationChanged(void *context, const Config &cfg);

#ifndef DO_NOT_USE_GAME_SDK
	static std::unique_ptr<discord::Core> CreateDiscordCore();

	std::unique_ptr<discord::Core> m_DiscordCore;
	void RunDiscordCallbacks();
#endif

	ConfigManager m_Config;
	TaskbarAttributeWorker m_Worker;
	StartupManager m_Startup;
	MainAppWindow m_AppWindow;

	winrt::Windows::System::DispatcherQueueController m_DispatcherController;
	XamlContentManager m_Xaml;

	void CreateWelcomePage(bool hasPackageIdentity);
	winrt::fire_and_forget LicenseAcceptedCallback(bool hasPackageIdentity, bool startupState);
	Application(HINSTANCE hInst, bool hasPackageIdentity, bool fileExists);

public:
	Application(HINSTANCE hInst, bool hasPackageIdentity) : Application(hInst, hasPackageIdentity, false) { }

	static void OpenDonationPage();
	static void OpenTipsPage();

	void OpenDiscordServer();
	void EditConfigFile();

	constexpr ConfigManager &GetConfigManager() noexcept { return m_Config; }
	constexpr StartupManager &GetStartupManager() noexcept { return m_Startup; }
	constexpr TaskbarAttributeWorker &GetWorker() noexcept { return m_Worker; }

	int Run();
	winrt::fire_and_forget Shutdown(int exitCode);
};
