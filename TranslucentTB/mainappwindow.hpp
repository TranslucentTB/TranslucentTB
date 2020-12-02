#pragma once
#include "arch.h"
#include "tray/traycontextmenu.hpp"
#include <cstddef>
#include <spdlog/common.h>
#include <tuple>
#include <windef.h>

#include "config/config.hpp"
#include "dynamicloader.hpp"
#include "resources/ids.h"
#include "managers/startupmanager.hpp"

class Application;

class MainAppWindow final : public TrayContextMenu {
private:
	Application &m_App;
	bool m_HideIconOverride;

	LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	void RefreshMenu() override;
	void AppearanceMenuRefresh(uint16_t group, const TaskbarAppearance &appearance);

	// Ok, logs enabled, has file, text, level button
	static std::tuple<bool, bool, bool, uint16_t, unsigned int> GetLogMenu();

	// User modifiable, enabled, text
	static std::tuple<bool, bool, uint16_t> GetAutostartMenu(const StartupManager &manager);

	void ClickHandler(unsigned int id) override;
	static TaskbarAppearance &AppearanceForGroup(Config &cfg, uint16_t group) noexcept;
	void AppearanceMenuHandler(uint16_t group, uint16_t offset, Config &cfg);
	void HideTrayHandler();
	static void AutostartMenuHandler(StartupManager &manager);

	void Exit();

public:
	MainAppWindow(Application &app, bool hideIconOverride, bool hideStartup, HINSTANCE hInstance, DynamicLoader &loader);

	void UpdateTrayVisibility(bool visible);
	void RemoveHideTrayIconOverride();

	static void CloseRemote() noexcept;
};
