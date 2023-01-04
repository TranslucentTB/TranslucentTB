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
		if (uMsg == m_NewInstanceMessage)
		{
			if (!m_App.BringWelcomeToFront())
			{
				SendNotification(IDS_ALREADY_RUNNING);
				m_App.GetWorker().ResetState(true);
			}

			return 0;
		}
		else
		{
			return TrayContextMenu::MessageHandler(uMsg, wParam, lParam);
		}
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
	trayPage.SetTaskbarSettings(txmp::TaskbarState::BatterySaver, txmp::OptionalTaskbarAppearance(settings.BatterySaverAppearance));

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

	trayPage.SetStartupState(m_App.GetStartupManager().GetState());
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
	m_ResetSettingsRequestedRevoker = menu.ResetSettingsRequested(winrt::auto_revoke, { this, &MainAppWindow::ResetSettingsRequested });
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
		if (state == txmp::TaskbarState::Desktop) [[unlikely]]
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
	std::unique_lock lock(m_PickerMutex);
	auto &pickerHost = m_ColorPickers.at(static_cast<std::size_t>(state));
	if (!pickerHost)
	{
		auto &appearance = GetConfigForState(state);

		using winrt::TranslucentTB::Xaml::Pages::ColorPickerPage;
		m_App.CreateXamlWindow<ColorPickerPage>(xaml_startup_position::mouse,
			[this, &appearance, &pickerHost, state, inner_lock = std::move(lock)](const ColorPickerPage &picker, BaseXamlPageHost *host) mutable
			{
				pickerHost = host;
				inner_lock.unlock();

				auto closeRevoker = picker.Closed(winrt::auto_revoke, [this, state, &pickerHost]
				{
					m_App.DispatchToMainThread([this, state, &pickerHost]() mutable
					{
						m_App.GetWorker().RemoveColorPreview(state);

						std::scoped_lock guard(m_PickerMutex);
						pickerHost = nullptr;
					});
				});

				picker.ChangesCommitted([this, state, &appearance, &pickerHost, revoker = std::move(closeRevoker)](const winrt::Windows::UI::Color &color) mutable
				{
					revoker.revoke(); // we're already doing this.

					m_App.DispatchToMainThread([this, state, color, &appearance, &pickerHost]() mutable
					{
						appearance.Color = color;
						m_App.GetWorker().RemoveColorPreview(state); // remove color preview implicitly refreshes config

						std::scoped_lock guard(m_PickerMutex);
						pickerHost = nullptr;
					});
				});

				picker.ColorChanged([this, state](const winrt::Windows::UI::Color &color)
				{
					m_App.DispatchToMainThread([this, state, color]
					{
						m_App.GetWorker().ApplyColorPreview(state, color);
					});
				});
			},
			state,
			appearance.Color);
	}
	else
	{
		SetForegroundWindow(pickerHost->handle());
	}
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
	if (!std::exchange(m_HideTrayWarningShown, true))
	{
		std::thread([this, msg = Localization::LoadLocalizedResourceString(IDS_HIDETRAY_DIALOG, hinstance())]()
		{
#ifdef _DEBUG
			SetThreadDescription(GetCurrentThread(), APP_NAME L" Hide Tray Message Box Thread");
#endif

			if (MessageBoxEx(Window::NullWindow, msg.c_str(), APP_NAME, MB_YESNO | MB_ICONINFORMATION | MB_SETFOREGROUND, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL)) == IDYES)
			{
				m_App.DispatchToMainThread([this]
				{
					m_HideIconOverride = true;
					Hide();
				});
			}
		}).detach();
	}
}

void MainAppWindow::ResetDynamicStateRequested()
{
	m_App.GetWorker().ResetState(true);
}

void MainAppWindow::CompactThunkHeapRequested()
{
	member_thunk::compact();
}

winrt::fire_and_forget MainAppWindow::StartupStateChanged()
{
	auto &manager = m_App.GetStartupManager();
	if (const auto state = manager.GetState())
	{
		switch (*state)
		{
			using enum winrt::Windows::ApplicationModel::StartupTaskState;

		case Disabled:
			co_await manager.Enable();
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
	m_App.Shutdown();
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
	case BatterySaver: return config.BatterySaverAppearance;
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
	m_HideIconOverride(hideIconOverride),
	m_HideTrayWarningShown(false),
	m_NewInstanceMessage(Window::RegisterMessage(WM_TTBNEWINSTANCESTARTED))
{
	RegisterMenuHandlers();

	ConfigurationChanged();
}

void MainAppWindow::ConfigurationChanged()
{
	const Config &config = m_App.GetConfigManager().GetConfig();

	UpdateTrayVisibility(!config.HideTray);
	SetXamlContextMenuOverride(config.UseXamlContextMenu);
}

void MainAppWindow::RemoveHideTrayIconOverride()
{
	m_HideIconOverride = false;
	UpdateTrayVisibility(!m_App.GetConfigManager().GetConfig().HideTray);
}

void MainAppWindow::PostNewInstanceNotification()
{
	if (const auto msg = Window::RegisterMessage(WM_TTBNEWINSTANCESTARTED))
	{
		if (const auto runningInstance = Window::Find(TRAY_WINDOW, APP_NAME))
		{
			AllowSetForegroundWindow(runningInstance.process_id());
			runningInstance.post_message(*msg);
		}
	}
}
