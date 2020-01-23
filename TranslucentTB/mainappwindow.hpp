#pragma once
#include "tray/traycontextmenu.hpp"
#include <filesystem>
#include <wil/filesystem.h>
#include <wil/resource.h>
#include <winrt/base.h>
#include <winrt/Windows.ApplicationModel.h>

#include "config/config.hpp"
#include "taskbar/taskbarattributeworker.hpp"

class MainAppWindow final : public TrayContextMenu {
private:
	std::filesystem::path m_ConfigPath;
	Config m_Config;

	TaskbarAttributeWorker m_Worker;

	UINT m_FileChangedMessage;
	wil::unique_folder_change_reader m_FolderWatcher;

	bool m_HasPackageIdentity;
	wil::srwlock m_TaskLock;
	winrt::Windows::ApplicationModel::StartupTask m_StartupTask;

	void SetupFolderWatch();
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
	TaskbarAppearance &AppearanceForGroup(uint16_t group);
	void AppearanceMenuHandler(uint8_t offset, TaskbarAppearance &appearance, bool &b);
	void HideTrayHandler();
	void AutostartMenuHandler();

	void Exit(bool save);

	void VerbosityChanged();
	void AppearanceChanged();
	void TrayIconChanged();
	void ConfigurationReloaded();

	inline void Save()
	{
		m_Config.Save(m_ConfigPath);
	}

	static constexpr void InvertBool(bool &b)
	{
		b = !b;
	}

public:
	MainAppWindow(std::filesystem::path configPath, bool hasPackageIdentity, HINSTANCE hInstance);
	WPARAM Run();

	static void CloseRemote();
};
