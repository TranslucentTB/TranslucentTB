#pragma once
#include "tray/traycontextmenu.hpp"
#include <discord-game-sdk/core.h>
#include <exception>
#include <filesystem>
#include <memory>
#include <string_view>
#include <wil/filesystem.h>
#include <wil/resource.h>
#include <winrt/base.h>
#include <winrt/TranslucentTB.Xaml.h>
#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.UI.Xaml.Hosting.h>

#include "config/config.hpp"
#include "folderwatcher.hpp"
#include "taskbar/taskbarattributeworker.hpp"
#include "uwp/xamlpagehost.hpp"

// TODO Might wanna split this up in different classes...
class MainAppWindow final : public TrayContextMenu {
private:
	bool m_HasPackageIdentity;

	std::filesystem::path m_ConfigPath;
	Config m_Config;

	TaskbarAttributeWorker m_Worker;
	FolderWatcher m_Watcher;

	wil::srwlock m_TaskLock;
	winrt::Windows::ApplicationModel::StartupTask m_StartupTask;

	winrt::TranslucentTB::Xaml::App m_App;
	std::vector<winrt::weak_ref<winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource>> m_XamlSources;

	std::unique_ptr<discord::Core> m_DiscordCore;

	void LoadStartupTask();

	LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	void RefreshMenu() override;
	void AppearanceMenuRefresh(uint16_t group, TaskbarAppearance &appearance, bool &b, bool controlsEnabled);
	void LogMenuRefresh();
	void AutostartMenuRefresh();

	inline void AppearanceMenuRefresh(uint16_t group, OptionalTaskbarAppearance &appearance)
	{
		AppearanceMenuRefresh(group, appearance, appearance.Enabled, true);
	}

	void ClickHandler(unsigned int id) override;
	TaskbarAppearance &AppearanceForGroup(uint16_t group) noexcept;
	void AppearanceMenuHandler(uint8_t offset, TaskbarAppearance &appearance, bool &b);
	void HideTrayHandler();
	void AutostartMenuHandler();

	void Exit(bool save);

	void VerbosityChanged();
	void AppearanceChanged();
	void TrayIconChanged();
	void ConfigurationReloaded();

	std::filesystem::path GetConfigPath();

	std::unique_ptr<discord::Core> CreateDiscordCore();
	void OpenDiscordServer();

	inline void Save()
	{
		m_Config.Save(m_ConfigPath);
	}

	template<typename T, typename... Args>
	inline XamlPageHost<T> *CreateXamlWindow(Args&&... args)
	{
		try
		{
			// Load XAML
			if (!m_App)
			{
				m_App = { };
			}

			const auto page = new XamlPageHost<T>(hinstance(), std::forward<Args>(args)...);
			m_XamlSources.push_back(page->source());
			return page;
		}
		HresultErrorCatch(spdlog::level::critical, L"Failed to open window");
	}

	static constexpr void InvertBool(bool &b)
	{
		b = !b;
	}

	static void WatcherCallback(void *context, DWORD, std::wstring_view fileName);

public:
	MainAppWindow(HINSTANCE hInstance);
	WPARAM Run();

	static void CloseRemote() noexcept;
};
