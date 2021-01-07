#pragma once
#include "arch.h"
#include <filesystem>
#include <memory>
#include <optional>
#include <windef.h>

#include "dynamicloader.hpp"
#include "managers/configmanager.hpp"
#include "mainappwindow.hpp"
#include "managers/startupmanager.hpp"
#include "taskbar/taskbarattributeworker.hpp"
#include "uwp/xamlthreadpool.hpp"

#ifndef DO_NOT_USE_GAME_SDK
#include <discord-game-sdk/core.h>
#endif

class Application final {
	static void ConfigurationChanged(void *context, const Config &cfg);

#ifndef DO_NOT_USE_GAME_SDK
	static std::unique_ptr<discord::Core> CreateDiscordCore();

	std::unique_ptr<discord::Core> m_DiscordCore;
	void RunDiscordCallbacks();
#endif
	DynamicLoader m_Loader;

	ConfigManager m_Config;
	TaskbarAttributeWorker m_Worker;
	StartupManager m_Startup;
	MainAppWindow m_AppWindow;

	winrt::Windows::System::DispatcherQueueController m_DispatcherController;
	XamlThreadPool m_Xaml;

	void CreateWelcomePage(bool hasPackageIdentity);
	winrt::fire_and_forget LicenseApprovedCallback(bool hasPackageIdentity, bool startupState);
	Application(HINSTANCE hInst, std::optional<std::filesystem::path> storageFolder, bool fileExists);

public:
	Application(HINSTANCE hInst, std::optional<std::filesystem::path> storageFolder) : Application(hInst, std::move(storageFolder), false) { }

	static void OpenDonationPage();
	static void OpenTipsPage();

	void OpenDiscordServer();
	void EditConfigFile();

	constexpr ConfigManager &GetConfigManager() noexcept { return m_Config; }
	constexpr StartupManager &GetStartupManager() noexcept { return m_Startup; }
	constexpr TaskbarAttributeWorker &GetWorker() noexcept { return m_Worker; }

	int Run();
	winrt::fire_and_forget Shutdown(int exitCode);

	template<typename T, typename Callback, typename... Args>
	void CreateXamlWindow(xaml_startup_position pos, Callback &&callback, Args&&... args)
	{
		m_Xaml.CreateXamlWindow<T>(pos, m_Loader.ShouldAppsUseDarkMode(), std::forward<Callback>(callback), std::forward<Args>(args)...);
	}

	template<typename Callback>
	void DispatchToMainThread(Callback &&callback)
	{
		m_DispatcherController.DispatcherQueue().TryEnqueue(std::forward<Callback>(callback));
	}
};
