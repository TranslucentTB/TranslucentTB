#pragma once
#include "arch.h"
#include <filesystem>
#include <memory>
#include <optional>
#include <windef.h>
#include "winrt.hpp"
#include "undefgetcurrenttime.h"
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.UI.Xaml.Hosting.h>
#include "redefgetcurrenttime.h"
#include <winrt/TranslucentTB.Xaml.h>

#include "dynamicloader.hpp"
#include "managers/configmanager.hpp"
#include "mainappwindow.hpp"
#include "managers/startupmanager.hpp"
#include "taskbar/taskbarattributeworker.hpp"
#include "uwp/dynamicdependency.hpp"
#include "uwp/xamlthreadpool.hpp"

class Application final {
	static void ConfigurationChanged(void *context);
	static winrt::TranslucentTB::Xaml::App CreateXamlApp();

	DynamicLoader m_Loader;
	DynamicDependency m_UwpCRTDep, m_WinUIDep;

	ConfigManager m_Config;
	TaskbarAttributeWorker m_Worker;
	StartupManager m_Startup;

	winrt::Windows::System::DispatcherQueueController m_DispatcherController;
	winrt::TranslucentTB::Xaml::App m_XamlApp;
	wuxh::WindowsXamlManager m_XamlManager;
	MainAppWindow m_AppWindow;

	XamlThreadPool m_Xaml;

	winrt::fire_and_forget CreateWelcomePage(wf::IAsyncOperation<bool> operation);
	Application(HINSTANCE hInst, std::optional<std::filesystem::path> storageFolder, bool fileExists);

public:
	Application(HINSTANCE hInst, std::optional<std::filesystem::path> storageFolder) : Application(hInst, std::move(storageFolder), false) { }

	static void OpenDonationPage();
	static void OpenTipsPage();
	static void OpenDiscordServer();

	constexpr ConfigManager &GetConfigManager() noexcept { return m_Config; }
	constexpr StartupManager &GetStartupManager() noexcept { return m_Startup; }
	constexpr TaskbarAttributeWorker &GetWorker() noexcept { return m_Worker; }

	int Run();
	winrt::fire_and_forget Shutdown(int exitCode);

	template<typename T, typename Callback, typename... Args>
	void CreateXamlWindow(xaml_startup_position pos, Callback &&callback, Args&&... args)
	{
		m_Xaml.CreateXamlWindow<T>(pos, std::forward<Callback>(callback), std::forward<Args>(args)...);
	}

	void DispatchToMainThread(const winrt::Windows::System::DispatcherQueueHandler &callback,
		winrt::Windows::System::DispatcherQueuePriority priority = winrt::Windows::System::DispatcherQueuePriority::Normal)
	{
		m_DispatcherController.DispatcherQueue().TryEnqueue(priority, callback);
	}
};
