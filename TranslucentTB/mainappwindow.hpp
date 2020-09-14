#pragma once
#include "arch.h"
#include "tray/traycontextmenu.hpp"
#include <cstddef>
#include <optional>
#include <spdlog/common.h>
#include <tuple>
#include <windef.h>

#include "config/config.hpp"
#include "resources/ids.h"
#include "startupmanager.hpp"

class Application;

class MainAppWindow final : public TrayContextMenu {
private:
	Application &m_App;
	bool m_HideIconOverride;

#ifndef DO_NOT_USE_GAME_SDK
	bool PreTranslateMessage(const MSG &msg) override;
#endif

	LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	void RefreshMenu() override;
	void AppearanceMenuRefresh(uint16_t group, const TaskbarAppearance &appearance, bool b, bool controlsEnabled);

	// Ok, logs enabled, has file, text, level button
	std::tuple<bool, bool, bool, uint16_t, unsigned int> GetLogMenu();

	// User modifiable, enabled, text
	std::tuple<bool, bool, uint16_t> GetAutostartMenu(const StartupManager &manager);

	inline void AppearanceMenuRefresh(uint16_t group, const OptionalTaskbarAppearance &appearance)
	{
		AppearanceMenuRefresh(group, appearance, appearance.Enabled, true);
	}

	void ClickHandler(unsigned int id) override;
	TaskbarAppearance &AppearanceForGroup(Config &cfg, uint16_t group) noexcept;
	void AppearanceMenuHandler(uint8_t offset, TaskbarAppearance &appearance, bool &b);
	void HideTrayHandler();
	void AutostartMenuHandler();

	void Exit(bool save);

public:
	MainAppWindow(Application &app, bool hideIconOverride, HINSTANCE hInstance);

	void UpdateTrayVisibility(bool visible);
	void RemoveHideTrayIconOverride();

	static void CloseRemote() noexcept;
};
