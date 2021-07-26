#include "mainappwindow.hpp"
#include <member_thunk/member_thunk.hpp>

#include "application.hpp"
#include "constants.hpp"
#include "localization.hpp"
#include "resources/ids.h"
#include "../ProgramLog/log.hpp"
#include "../ProgramLog/error/win32.hpp"

LRESULT MainAppWindow::MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CLOSE:
		Exit();
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
			Exit();
		}

		return 0;

	default:
		return TrayContextMenu::MessageHandler(uMsg, wParam, lParam);
	}
}

void MainAppWindow::RefreshMenu()
{
	const auto &trayPage = page();
	const auto &settings = m_App.GetConfigManager().GetConfig();

	trayPage.SetTaskbarSettings(txmp::TaskbarState::Desktop, settings.DesktopAppearance);
	trayPage.SetTaskbarSettings(txmp::TaskbarState::VisibleWindow, txmp::OptionalTaskbarAppearance(settings.VisibleWindowAppearance));
	trayPage.SetTaskbarSettings(txmp::TaskbarState::MaximisedWindow, txmp::OptionalTaskbarAppearance(settings.MaximisedWindowAppearance));
	trayPage.SetTaskbarSettings(txmp::TaskbarState::StartOpened, txmp::OptionalTaskbarAppearance(settings.StartOpenedAppearance));
	trayPage.SetTaskbarSettings(txmp::TaskbarState::SearchOpened, txmp::OptionalTaskbarAppearance(settings.SearchOpenedAppearance));
	trayPage.SetTaskbarSettings(txmp::TaskbarState::TaskViewOpened, txmp::OptionalTaskbarAppearance(settings.TaskViewOpenedAppearance));

	if (const auto sink = Log::GetSink())
	{
		trayPage.SetLogLevel(static_cast<txmp::LogLevel>(sink->level()));
		trayPage.SinkState(static_cast<txmp::LogSinkState>(sink->state()));
	}
	else
	{
		trayPage.SinkState(txmp::LogSinkState::Failed);
	}

	trayPage.SetDisableSavingSettings(settings.DisableSaving);

	const auto state = m_App.GetStartupManager().GetState();
	trayPage.SetStartupState(state ? wf::IReference(*state) : nullptr);
}

void MainAppWindow::RegisterMenuHandlers()
{
	const auto &menu = page();
	m_TaskbarSettingsChangedRevoker = menu.TaskbarSettingsChanged(winrt::auto_revoke, { this, &MainAppWindow::TaskbarSettingsChanged });
	m_ColorRequestedRevoker = menu.ColorRequested(winrt::auto_revoke, { this, &MainAppWindow::ColorRequested });

	m_OpenLogFileRequestedRevoker = menu.OpenLogFileRequested(winrt::auto_revoke, { this, &MainAppWindow::OpenLogFileRequested });
	m_LogLevelChangedRevoker = menu.LogLevelChanged(winrt::auto_revoke, { this, &MainAppWindow::LogLevelChanged });
	m_DumpDynamicStateRequestedRevoker = menu.DumpDynamicStateRequested(winrt::auto_revoke, { this, &MainAppWindow::DumpDynamicStateRequested });
	m_EditSettingsRequestedRevoker = menu.EditSettingsRequested(winrt::auto_revoke, { this, &MainAppWindow::EditSettingsRequested });
	m_ResetDynamicStateRequestedRevoker = menu.ResetDynamicStateRequested(winrt::auto_revoke, { this, &MainAppWindow::ResetDynamicStateRequested });
	m_DisableSavingSettingsChangedRevoker = menu.DisableSavingSettingsChanged(winrt::auto_revoke, { this, &MainAppWindow::DisableSavingSettingsChanged });
	m_HideTrayRequestedRevoker = menu.HideTrayRequested(winrt::auto_revoke, { this, &MainAppWindow::HideTrayRequested });
	m_ResetDynamicStateRequestedRevoker = menu.ResetDynamicStateRequested(winrt::auto_revoke, { this, &MainAppWindow::ResetDynamicStateRequested });
	m_CompactThunkHeapRequestedRevoker = menu.CompactThunkHeapRequested(winrt::auto_revoke, MainAppWindow::CompactThunkHeapRequested);

	m_StartupStateChangedRevoker = menu.StartupStateChanged(winrt::auto_revoke, { this, &MainAppWindow::StartupStateChanged });
	m_TipsAndTricksRequestedRevoker = menu.TipsAndTricksRequested(winrt::auto_revoke, MainAppWindow::TipsAndTricksRequested);
	m_AboutRequestedRevoker = menu.AboutRequested(winrt::auto_revoke, { this, &MainAppWindow::AboutRequested });
	m_ExitRequestedRevoker = menu.ExitRequested(winrt::auto_revoke, { this, &MainAppWindow::Exit });
}

void MainAppWindow::TaskbarSettingsChanged(const txmp::TaskbarState &state, const txmp::TaskbarAppearance &appearance)
{
	auto &config = GetConfigForState(state);

	// restore color because the context menu doesn't transmit that info
	appearance.Color(config.Color);

	if (const auto optAppearance = appearance.try_as<txmp::OptionalTaskbarAppearance>())
	{
		if (state == txmp::TaskbarState::Desktop)
		{
			throw std::invalid_argument("Desktop appearance is not optional");
		}

		static_cast<OptionalTaskbarAppearance &>(config) = optAppearance;
	}
	else
	{
		config = appearance;
	}

	m_App.GetWorker().ConfigurationChanged();
}

void MainAppWindow::ColorRequested(const txmp::TaskbarState &state)
{
	auto &appearance = GetConfigForState(state);

	using winrt::TranslucentTB::Xaml::Pages::ColorPickerPage;
	m_App.CreateXamlWindow<ColorPickerPage>(xaml_startup_position::mouse, [this, &appearance](const ColorPickerPage &picker)
	{
		picker.ChangesCommitted([this, &appearance](const winrt::Windows::UI::Color &color)
		{
			m_App.DispatchToMainThread([this, &appearance, color]() mutable
			{
				appearance.Color = color;
				m_App.GetWorker().ConfigurationChanged();
			});
		});
	}, L"TEST", appearance.Color);
	// TODO: use a localized string
}

void MainAppWindow::OpenLogFileRequested()
{
	if (const auto sink = Log::GetSink())
	{
		HresultVerify(win32::EditFile(sink->file()), spdlog::level::err, L"Failed to open log file.");
	}
}

void MainAppWindow::LogLevelChanged(const txmp::LogLevel &level)
{
	const auto spdlogLevel = static_cast<spdlog::level::level_enum>(level);

	auto &configManager = m_App.GetConfigManager();
	configManager.GetConfig().LogVerbosity = spdlogLevel;
	configManager.UpdateVerbosity();
}

void MainAppWindow::DumpDynamicStateRequested()
{
	m_App.GetWorker().DumpState();
}

void MainAppWindow::EditSettingsRequested()
{
	m_App.GetConfigManager().EditConfigFile();
}

void MainAppWindow::ResetSettingsRequested()
{
	auto &manager = m_App.GetConfigManager();
	manager.GetConfig() = { };

	manager.UpdateVerbosity();
	m_App.GetWorker().ConfigurationChanged();
	ConfigurationChanged();
}

void MainAppWindow::DisableSavingSettingsChanged(bool disabled) noexcept
{
	m_App.GetConfigManager().GetConfig().DisableSaving = disabled;
}

void MainAppWindow::HideTrayRequested()
{
	if (MessageBoxEx(
			Window::NullWindow,
			Localization::LoadLocalizedResourceString(IDS_HIDETRAY_DIALOG, hinstance()).c_str(),
			APP_NAME,
			MB_YESNO | MB_ICONINFORMATION | MB_SETFOREGROUND,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL)
		) == IDYES)
	{
		m_HideIconOverride = true;
		Hide();
	}
}

void MainAppWindow::ResetDynamicStateRequested()
{
	m_App.GetWorker().ResetState();
}

void MainAppWindow::CompactThunkHeapRequested()
{
	member_thunk::compact();
}

void MainAppWindow::StartupStateChanged()
{
	auto &manager = m_App.GetStartupManager();
	if (const auto state = manager.GetState())
	{
		switch (*state)
		{
			using enum winrt::Windows::ApplicationModel::StartupTaskState;

		case Disabled:
			manager.Enable();
			break;

		case Enabled:
			manager.Disable();
			break;

		case DisabledByUser:
			StartupManager::OpenSettingsPage();
			break;

		default:
			MessagePrint(spdlog::level::err, L"Cannot change startup state because it is locked by external factors (for example Group Policy).");
			break;
		}
	}
}

void MainAppWindow::TipsAndTricksRequested()
{
	Application::OpenTipsPage();
}

void MainAppWindow::AboutRequested()
{
	//m_App.CreateXamlWindow<winrt::TranslucentTB::Xaml::Pages::AboutPage>(xaml_startup_position::center, [](const auto &) { });
}

void MainAppWindow::Exit()
{
	m_App.GetConfigManager().SaveConfig();
	m_App.Shutdown(0);
}

TaskbarAppearance &MainAppWindow::GetConfigForState(const txmp::TaskbarState &state)
{
	auto &config = m_App.GetConfigManager().GetConfig();
	switch (state)
	{
		using enum txmp::TaskbarState;

	case Desktop: return config.DesktopAppearance;
	case VisibleWindow: return config.VisibleWindowAppearance;
	case MaximisedWindow: return config.MaximisedWindowAppearance;
	case StartOpened: return config.StartOpenedAppearance;
	case SearchOpened: return config.SearchOpenedAppearance;
	case TaskViewOpened: return config.TaskViewOpenedAppearance;
	default: throw std::invalid_argument("Unknown taskbar state");
	}
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

MainAppWindow::MainAppWindow(Application &app, bool hideIconOverride, bool hasPackageIdentity, HINSTANCE hInstance, DynamicLoader &loader) :
	// make the window topmost so that the context menu shows correctly
	MessageWindow(TRAY_WINDOW, APP_NAME, hInstance, WS_POPUP, WS_EX_TOPMOST | WS_EX_NOREDIRECTIONBITMAP),
	TrayContextMenu(TRAY_GUID, MAKEINTRESOURCE(IDI_TRAYWHITEICON), MAKEINTRESOURCE(IDI_TRAYBLACKICON), loader, hasPackageIdentity),
	m_App(app),
	m_HideIconOverride(hideIconOverride)
{
	RegisterMenuHandlers();

	ConfigurationChanged();
}

void MainAppWindow::ConfigurationChanged()
{
	const Config &config = m_App.GetConfigManager().GetConfig();

	UpdateTrayVisibility(!config.HideTray);
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
