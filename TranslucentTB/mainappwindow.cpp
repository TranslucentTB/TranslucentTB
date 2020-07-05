#include "mainappwindow.hpp"

#include "application.hpp"
#include "constants.hpp"
#include "undoc/dynamicloader.hpp"
#include "../ProgramLog/log.hpp"
#include "../ProgramLog/error/win32.hpp"

#ifndef DO_NOT_USE_GAME_SDK
bool MainAppWindow::PreTranslateMessage(const MSG &)
{
	m_App.RunDiscordCallbacks();
	return false;
}
#endif

LRESULT MainAppWindow::MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CLOSE:
		Exit(true);
		return 1;

	case WM_QUERYENDSESSION:
		if (lParam & ENDSESSION_CLOSEAPP)
		{
			// The app is being queried if it can close for an update.
			RegisterApplicationRestart(nullptr, 0);
		}
		return 1;

	case WM_ENDSESSION:
		if (wParam)
		{
			// The app can be killed after processing this message, but we'll try doing it gracefully
			Exit(true);
		}

		return 0;

	default:
		return TrayContextMenu::MessageHandler(uMsg, wParam, lParam);
	}
}

void MainAppWindow::RefreshMenu()
{
	const auto &cfg = m_App.GetConfigManager().GetConfig();

	// TODO AppearanceMenuRefresh(ID_GROUP_DESKTOP, m_Config.DesktopAppearance, m_Config.UseRegularAppearanceWhenPeeking, false);
	AppearanceMenuRefresh(ID_GROUP_VISIBLE, cfg.VisibleWindowAppearance);
	AppearanceMenuRefresh(ID_GROUP_MAXIMISED, cfg.MaximisedWindowAppearance);
	AppearanceMenuRefresh(ID_GROUP_START, cfg.StartOpenedAppearance);
	AppearanceMenuRefresh(ID_GROUP_CORTANA, cfg.CortanaOpenedAppearance);
	AppearanceMenuRefresh(ID_GROUP_TIMELINE, cfg.TimelineOpenedAppearance);

	const auto [ok, logsEnabled, hasFile, logText, levelButton] = GetLogMenu();
	EnableItem(ID_SUBMENU_LOG, ok);
	CheckItem(ID_SUBMENU_LOG, logsEnabled);
	EnableItem(ID_OPENLOG, hasFile);
	SetText(ID_OPENLOG, logText);
	CheckRadio(ID_LOG_TRACE, ID_LOG_OFF, levelButton);

	CheckItem(ID_DISABLESAVING, cfg.DisableSaving);

	if (const auto &manager = m_App.GetStartupManager())
	{
		const auto [userModifiable, enabled, autostartText] = GetAutostartMenu(*manager);

		CheckItem(ID_AUTOSTART, enabled);
		EnableItem(ID_AUTOSTART, userModifiable);
		SetText(ID_AUTOSTART, autostartText);
	}
}

void MainAppWindow::AppearanceMenuRefresh(uint16_t group, const TaskbarAppearance &appearance, bool b, bool controlsEnabled)
{
	CheckItem(ID_TYPE_ACTIONS + group + ID_OFFSET_ENABLED, b);

	EnableItem(ID_TYPE_ACTIONS + group + ID_OFFSET_COLOR, controlsEnabled && !b ? false : appearance.Accent != ACCENT_NORMAL);
	if (controlsEnabled)
	{
		EnableItem(ID_TYPE_RADIOS + group + ID_OFFSET_NORMAL, b);
		EnableItem(ID_TYPE_RADIOS + group + ID_OFFSET_OPAQUE, b);
		EnableItem(ID_TYPE_RADIOS + group + ID_OFFSET_CLEAR, b);
		EnableItem(ID_TYPE_RADIOS + group + ID_OFFSET_BLUR, b);
		EnableItem(ID_TYPE_RADIOS + group + ID_OFFSET_ACRYLIC, b);
	}

	CheckRadio(
		ID_TYPE_RADIOS + group + ID_OFFSET_NORMAL,
		ID_TYPE_RADIOS + group + ID_OFFSET_ACRYLIC,
		ID_TYPE_RADIOS + group + appearance.Accent
	);
}

std::tuple<bool, bool, bool, uint16_t, unsigned int> MainAppWindow::GetLogMenu()
{
	const auto sink = Log::GetSink();
	if (!sink)
	{
		return LOG_ERROR;
	}

	const bool hasFile = sink->opened();
	if (hasFile)
	{
		return GetLogSuccess(hasFile, sink->level(), IDS_OPENLOG_NORMAL);
	}
	else if (!hasFile && !sink->tried())
	{
		return GetLogSuccess(hasFile, sink->level(), IDS_OPENLOG_EMPTY);
	}
	else
	{
		return LOG_ERROR;
	}
}

std::tuple<bool, bool, uint16_t> MainAppWindow::GetAutostartMenu(const StartupManager &manager)
{
	const auto state = manager.GetState();
	if (!state)
	{
		return { false, false, IDS_AUTOSTART_ERROR };
	}

	switch (*state)
	{
		using winrt::Windows::ApplicationModel::StartupTaskState;

	case StartupTaskState::Disabled:
		return { true, false, IDS_AUTOSTART_NORMAL };

	case StartupTaskState::DisabledByPolicy:
		return { false, false, IDS_AUTOSTART_DISABLED_GPEDIT };

	case StartupTaskState::DisabledByUser:
		return { false, false, IDS_AUTOSTART_DISABLED_TASKMGR };

	case StartupTaskState::Enabled:
		return { true, true, IDS_AUTOSTART_NORMAL };

	case StartupTaskState::EnabledByPolicy:
		return { false, true, IDS_AUTOSTART_ENABLED_GPEDIT };

	default:
		return { false, false, IDS_AUTOSTART_UNKNOWN };
	}
}

void MainAppWindow::ClickHandler(unsigned int id)
{
	auto &cfgManager = m_App.GetConfigManager();
	auto &cfg = cfgManager.GetConfig();
	auto &worker = m_App.GetWorker();

	const uint16_t group = id & ID_GROUP_MASK;

	switch (id & ID_TYPE_MASK)
	{
	case ID_TYPE_RADIOS:
	{
		const uint8_t offset = id & ID_OFFSET_MASK;
		switch (group)
		{
		case ID_GROUP_DESKTOP:
		case ID_GROUP_VISIBLE:
		case ID_GROUP_MAXIMISED:
		case ID_GROUP_START:
		case ID_GROUP_CORTANA:
		case ID_GROUP_TIMELINE:
			AppearanceForGroup(cfg, group).Accent = static_cast<ACCENT_STATE>(offset);
			worker.ConfigurationChanged();
			break;
		case ID_GROUP_LOG:
			cfg.LogVerbosity = static_cast<spdlog::level::level_enum>(offset);
			cfgManager.UpdateVerbosity();
			break;
		}
		break;
	}

	case ID_TYPE_ACTIONS:
		switch (group)
		{
		case ID_GROUP_DESKTOP:
		case ID_GROUP_VISIBLE:
		case ID_GROUP_MAXIMISED:
		case ID_GROUP_START:
		case ID_GROUP_CORTANA:
		case ID_GROUP_TIMELINE:
			// TODO
			break;
		default:
			switch (id)
			{
			case ID_OPENLOG:
				HresultVerify(win32::EditFile(Log::GetSink()->file()), spdlog::level::err, L"Failed to open log file.");
				break;
			case ID_EDITSETTINGS:
				m_App.EditConfigFile();
				break;
			case ID_RETURNTODEFAULTSETTINGS:
				cfg = { };
				cfgManager.UpdateVerbosity();
				UpdateTrayVisibility(!cfg.HideTray);
				worker.ConfigurationChanged();
				break;
			case ID_DISABLESAVING:
				cfg.DisableSaving = !cfg.DisableSaving;
				break;
			case ID_HIDETRAY:
				HideTrayHandler();
				break;
			case ID_DUMPWORKER:
				worker.DumpState();
				break;
			case ID_RESETWORKER:
				worker.ResetState();
				break;
			case ID_ABOUT:
				MessageBox(nullptr, L"TODO", L"TODO", 0); // TODO
				break;
			case ID_AUTOSTART:
				AutostartMenuHandler();
				break;
			case ID_TIPS:
				m_App.OpenTipsPage();
				break;
			case ID_EXITWITHOUTSAVING:
			case ID_EXIT:
				Exit(id == ID_EXIT);
				break;
			}
			break;
		}
		break;
	}
}

TaskbarAppearance &MainAppWindow::AppearanceForGroup(Config &cfg, uint16_t group) noexcept
{
	switch (group)
	{
	case ID_GROUP_DESKTOP: return cfg.DesktopAppearance;
	case ID_GROUP_VISIBLE: return cfg.VisibleWindowAppearance;
	case ID_GROUP_MAXIMISED: return cfg.MaximisedWindowAppearance;
	case ID_GROUP_START: return cfg.StartOpenedAppearance;
	case ID_GROUP_CORTANA: return cfg.CortanaOpenedAppearance;
	case ID_GROUP_TIMELINE: return cfg.TimelineOpenedAppearance;
	default:
		assert(false);
		__assume(0);
	}
}

void MainAppWindow::AppearanceMenuHandler(uint8_t offset, [[maybe_unused]] TaskbarAppearance &appearance, bool &b)
{
	if (offset == ID_OFFSET_ENABLED)
	{
		b = !b;
		m_App.GetWorker().ConfigurationChanged();
	}
	else if (offset == ID_OFFSET_COLOR)
	{
		// TODO: color, remove maybe_unused
	}
}

void MainAppWindow::HideTrayHandler()
{
	if (MessageBoxEx(
			Window::NullWindow,
			L"This change is temporary and will be lost the next time TranslucentTB is started.\n\n"
			L"To make this permanent, edit the configuration file using \"Advanced\" > \"Edit settings\".\n\n"
			L"Are you sure you want to proceed?",
			APP_NAME,
			MB_YESNO | MB_ICONINFORMATION | MB_SETFOREGROUND,
			MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL)
		) == IDYES)
	{
		m_HideIconOverride = true;
		Hide();
	}
}

void MainAppWindow::AutostartMenuHandler()
{
	auto &manager  = *m_App.GetStartupManager();
	if (const auto state = manager.GetState())
	{
		switch (*state)
		{
			using winrt::Windows::ApplicationModel::StartupTaskState;

		case StartupTaskState::Disabled:
			manager.Enable();
			break;

		case StartupTaskState::Enabled:
			manager.Disable();
			break;

		default:
			MessagePrint(spdlog::level::err, L"Cannot change startup state because it is locked by external factors (for example Task Manager or Group Policy).");
			break;
		}
	}
}

void MainAppWindow::Exit(bool save)
{
	if (save)
	{
		m_App.GetConfigManager().SaveConfig();
	}

	PostQuitMessage(0);
}

MainAppWindow::MainAppWindow(Application &app, bool hideIconOverride, HINSTANCE hInstance) :
	TrayContextMenu(TRAY_GUID, TRAY_WINDOW, APP_NAME, MAKEINTRESOURCE(IDI_TRAYWHITEICON), MAKEINTRESOURCE(IDI_TRAYBLACKICON), MAKEINTRESOURCE(IDR_TRAY_MENU), hInstance),
	m_App(app),
	m_HideIconOverride(hideIconOverride)
{
	if (!m_App.GetStartupManager())
	{
		RemoveItem(ID_AUTOSTART);
	}

	if (DynamicLoader::uxtheme())
	{
		if (const auto spam = DynamicLoader::SetPreferredAppMode())
		{
			spam(PreferredAppMode::AllowDark);
		}
	}

	// Shows the tray icon if not disabled.
	UpdateTrayVisibility(!m_App.GetConfigManager().GetConfig().HideTray);
}

void MainAppWindow::UpdateTrayVisibility(bool visible)
{
	if (!m_HideIconOverride && visible)
	{
		Show();
	}
	else
	{
		Hide();
	}
}

void MainAppWindow::RemoveHideTrayIconOverride()
{
	m_HideIconOverride = false;
	UpdateTrayVisibility(!m_App.GetConfigManager().GetConfig().HideTray);
}

void MainAppWindow::CloseRemote() noexcept
{
	Window::Find(TRAY_WINDOW, APP_NAME).send_message(WM_CLOSE);
}
