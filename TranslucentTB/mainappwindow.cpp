#include "mainappwindow.hpp"
#include <algorithm>
#include <dwmapi.h>
#include <chrono>
#include <member_thunk/member_thunk.hpp>
#include <shellapi.h>
#include <thread>
#include <winreg.h>
#include <tlhelp32.h>
#include <winuser.h>

#include "application.hpp"
#include "constants.hpp"
#include "localization.hpp"
#include "resources/ids.h"
#include "../ProgramLog/log.hpp"
#include "../ProgramLog/error/win32.hpp"

namespace
{
	std::optional<bool> IsPrimaryTaskbarAreaDark()
	{
		const int width = GetSystemMetrics(SM_CXSCREEN);
		const int height = GetSystemMetrics(SM_CYSCREEN);
		if (width <= 0 || height <= 0)
		{
			return std::nullopt;
		}

		HDC screen = GetDC(nullptr);
		if (!screen)
		{
			return std::nullopt;
		}

		long brightnessSum = 0;
		int samples = 0;
		for (int yStep = 1; yStep <= 4; ++yStep)
		{
			const int y = height - 40 + (40 * yStep / 5);
			for (int xStep = 1; xStep <= 32; ++xStep)
			{
				const COLORREF color = GetPixel(screen, width * xStep / 33, y);
				if (color == CLR_INVALID)
				{
					ReleaseDC(nullptr, screen);
					return std::nullopt;
				}

				brightnessSum += (GetRValue(color) * 299 + GetGValue(color) * 587 + GetBValue(color) * 114) / 1000;
				++samples;
			}
		}

		ReleaseDC(nullptr, screen);
		return samples > 0 && brightnessSum / samples < 128;
	}

	void SetSystemTaskbarTheme(bool light)
	{
		HKEY key;
		if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr) == ERROR_SUCCESS)
		{
			const DWORD value = light ? 1 : 0;
			RegSetValueExW(key, L"SystemUsesLightTheme", 0, REG_DWORD, reinterpret_cast<const BYTE *>(&value), sizeof(value));
			RegCloseKey(key);
		}

		DWORD_PTR ignored;
		SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, reinterpret_cast<LPARAM>(L"ImmersiveColorSet"), SMTO_ABORTIFHUNG, 1000, &ignored);
		SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, reinterpret_cast<LPARAM>(L"WindowsThemeElement"), SMTO_ABORTIFHUNG, 1000, &ignored);
	}

	void ReapplySystemTaskbarTheme()
	{
		DWORD value = 0;
		DWORD size = sizeof(value);
		if (RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", L"SystemUsesLightTheme", RRF_RT_REG_DWORD, nullptr, &value, &size) == ERROR_SUCCESS)
		{
			SetSystemTaskbarTheme(value != 0);
		}
	}

	void NudgeSystemTaskbarTheme()
	{
		DWORD value = 0;
		DWORD size = sizeof(value);
		if (RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", L"SystemUsesLightTheme", RRF_RT_REG_DWORD, nullptr, &value, &size) != ERROR_SUCCESS)
		{
			return;
		}

		SetSystemTaskbarTheme(value == 0);
		DwmFlush();
		std::this_thread::sleep_for(std::chrono::milliseconds(75));
		SetSystemTaskbarTheme(value != 0);
	}

	BOOL CALLBACK CollectDialogText(HWND window, LPARAM parameter)
	{
		auto text = reinterpret_cast<std::wstring *>(parameter);
		const int length = GetWindowTextLengthW(window);
		if (length > 0)
		{
			std::wstring childText(length + 1, L'\0');
			GetWindowTextW(window, childText.data(), length + 1);
			text->append(childText.c_str());
		}
		return TRUE;
	}

	void CloseTaskbarAutoHideConflictDialog()
	{
		const HWND dialog = FindWindowW(L"#32770", L"Taskbar");
		if (!dialog)
		{
			return;
		}

		std::wstring content;
		EnumChildWindows(dialog, CollectDialogText, reinterpret_cast<LPARAM>(&content));
		if (content.find(L"A toolbar is already hidden on this side of your screen") != std::wstring::npos)
		{
			PostMessageW(dialog, WM_COMMAND, IDOK, 0);
		}
	}

	void SetTaskbarAutoHide()
	{
		APPBARDATA data = { .cbSize = sizeof(data), .hWnd = FindWindowW(L"Shell_TrayWnd", nullptr), .lParam = ABS_AUTOHIDE | ABS_ALWAYSONTOP };
		SHAppBarMessage(ABM_SETSTATE, &data);
	}

	void RefreshTaskbar()
	{
		const HWND taskbar = FindWindowW(L"Shell_TrayWnd", nullptr);
		if (!taskbar)
		{
			return;
		}

		// Repaint Explorer's taskbar without tearing down Explorer or its windows.
		DwmFlush();
		SendMessageTimeoutW(taskbar, WM_DWMCOMPOSITIONCHANGED, 0, 0, SMTO_ABORTIFHUNG, 500, nullptr);
		RedrawWindow(taskbar, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN | RDW_UPDATENOW);
		SetWindowPos(taskbar, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
		DwmFlush();
	}
}

LRESULT MainAppWindow::MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_DISPLAYCHANGE:
	case WM_DEVICECHANGE:
		if (m_App.GetConfigManager().GetConfig().KeepAutoHide)
		{
			ScheduleAutoHideRecovery();
		}
		return TrayContextMenu::MessageHandler(uMsg, wParam, lParam);

	case WM_POWERBROADCAST:
		if ((wParam == PBT_APMRESUMEAUTOMATIC || wParam == PBT_APMRESUMESUSPEND) && m_App.GetConfigManager().GetConfig().KeepAutoHide)
		{
			ScheduleAutoHideRecovery();
		}
		return TrayContextMenu::MessageHandler(uMsg, wParam, lParam);

	case WM_HOTKEY:
		if (wParam == RESET_STATE_GLOBAL_HOTKEY_ID)
		{
			ResetDynamicStateRequested();
		}
		return 0;

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
				SendNotification(IDS_ALREADY_RUNNING, NIF_REALTIME, NIIF_INFO);
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

	const auto type = m_App.GetWorker().GetType();
	trayPage.SetTaskbarType(type == TaskbarType::Classic ? txmp::TaskbarType::Classic : txmp::TaskbarType::XAML);
	trayPage.IsBlurSupported(type == TaskbarType::XAML ? true : m_App.GetWorker().IsBlurAccentStateSupported());

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
	trayPage.SetAutoDarkLightEnabled(settings.AutoDarkLight);
	trayPage.SetAutoDarkLightInterval(settings.AutoDarkLightIntervalMs);
	trayPage.SetKeepAutoHideEnabled(settings.KeepAutoHide);

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
	m_ResetDynamicStateRequestedRevoker = menu.ResetDynamicStateRequested(winrt::auto_revoke, { this, &MainAppWindow::ResetDynamicStateRequested });
	m_CompactThunkHeapRequestedRevoker = menu.CompactThunkHeapRequested(winrt::auto_revoke, MainAppWindow::CompactThunkHeapRequested);

	m_StartupStateChangedRevoker = menu.StartupStateChanged(winrt::auto_revoke, { this, &MainAppWindow::StartupStateChanged });
	m_AutoDarkLightChangedRevoker = menu.AutoDarkLightChanged(winrt::auto_revoke, { this, &MainAppWindow::AutoDarkLightChanged });
	m_AutoDarkLightIntervalChangedRevoker = menu.AutoDarkLightIntervalChanged(winrt::auto_revoke, { this, &MainAppWindow::AutoDarkLightIntervalChanged });
	m_KeepAutoHideChangedRevoker = menu.KeepAutoHideChanged(winrt::auto_revoke, { this, &MainAppWindow::KeepAutoHideChanged });
	m_FixTaskbarRequestedRevoker = menu.FixTaskbarRequested(winrt::auto_revoke, { this, &MainAppWindow::FixTaskbarRequested });
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

void MainAppWindow::AutoDarkLightChanged(bool enabled)
{
	m_App.GetConfigManager().GetConfig().AutoDarkLight = enabled;
	m_App.GetConfigManager().SaveConfig();
	UpdateAutoDarkLightWorker();
}

void MainAppWindow::AutoDarkLightIntervalChanged(int32_t intervalMs)
{
	if (intervalMs != 100 && intervalMs != 250 && intervalMs != 500 && intervalMs != 1000 && intervalMs != 2000)
	{
		return;
	}

	m_App.GetConfigManager().GetConfig().AutoDarkLightIntervalMs = intervalMs;
	m_App.GetConfigManager().SaveConfig();
	if (m_AutoDarkLightWorker.joinable())
	{
		m_AutoDarkLightWorker.request_stop();
		m_AutoDarkLightWorker.join();
	}
	UpdateAutoDarkLightWorker();
	RefreshMenu();
}

void MainAppWindow::KeepAutoHideChanged(bool enabled)
{
	m_App.GetConfigManager().GetConfig().KeepAutoHide = enabled;
	m_App.GetConfigManager().SaveConfig();
	if (enabled)
	{
		ScheduleAutoHideRecovery();
	}
}

void MainAppWindow::FixTaskbarRequested()
{
	m_App.GetWorker().ConfigurationChanged();
	ReapplySystemTaskbarTheme();
	NudgeSystemTaskbarTheme();
	RefreshTaskbar();
	if (m_App.GetConfigManager().GetConfig().KeepAutoHide)
	{
		ScheduleAutoHideRecovery();
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
	m_NewInstanceMessage(Window::RegisterMessage(WM_TTBNEWINSTANCESTARTED))
{
	// Register global hotkey for resetting dynamic state
	const BOOL hotkeyRegistered = RegisterHotKey(
		handle(),
		RESET_STATE_GLOBAL_HOTKEY_ID,
		MOD_CONTROL | MOD_ALT | MOD_SHIFT | MOD_NOREPEAT,
		VK_F1
	);
	if (!hotkeyRegistered)
	{
		LastErrorHandle(spdlog::level::warn, L"Unable to register global hotkey for dynamic state reset");
	}

	RegisterMenuHandlers();

	ConfigurationChanged();
}

MainAppWindow::~MainAppWindow()
{
	if (m_AutoDarkLightWorker.joinable())
	{
		m_AutoDarkLightWorker.request_stop();
		m_AutoDarkLightWorker.join();
	}
	if (m_AutoHideRecoveryWorker.joinable())
	{
		m_AutoHideRecoveryWorker.request_stop();
		m_AutoHideRecoveryWorker.join();
	}

	// Unregister the global hotkey
	UnregisterHotKey(handle(), RESET_STATE_GLOBAL_HOTKEY_ID);
}

void MainAppWindow::ConfigurationChanged()
{
	const Config &config = m_App.GetConfigManager().GetConfig();

	UpdateTrayVisibility(!config.HideTray.value_or(false));
	SetXamlContextMenuOverride(config.UseXamlContextMenu);
	UpdateAutoDarkLightWorker();
}

void MainAppWindow::UpdateAutoDarkLightWorker()
{
	const bool enabled = m_App.GetConfigManager().GetConfig().AutoDarkLight;
	if (enabled && !m_AutoDarkLightWorker.joinable())
	{
		const int interval = std::clamp(m_App.GetConfigManager().GetConfig().AutoDarkLightIntervalMs, 100, 2000);
		m_AutoDarkLightWorker = std::jthread(RunAutoDarkLightWorker, std::chrono::milliseconds(interval));
	}
	else if (!enabled && m_AutoDarkLightWorker.joinable())
	{
		m_AutoDarkLightWorker.request_stop();
		m_AutoDarkLightWorker.join();
	}
}

void MainAppWindow::ScheduleAutoHideRecovery()
{
	if (m_AutoHideRecoveryWorker.joinable())
	{
		m_AutoHideRecoveryWorker.request_stop();
		m_AutoHideRecoveryWorker.join();
	}
	m_AutoHideRecoveryWorker = std::jthread(RunAutoHideRecovery);
}

void MainAppWindow::RunAutoDarkLightWorker(std::stop_token stopToken, std::chrono::milliseconds interval)
{
	std::optional<bool> previousLight;
	while (!stopToken.stop_requested())
	{
		if (const auto dark = IsPrimaryTaskbarAreaDark())
		{
			const bool light = !*dark;
			if (previousLight != light)
			{
				SetSystemTaskbarTheme(light);
				previousLight = light;
			}
		}
		for (int elapsed = 0; elapsed < interval.count() && !stopToken.stop_requested(); elapsed += 50)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
	}
}

void MainAppWindow::RunAutoHideRecovery(std::stop_token stopToken)
{
	for (int attempt = 0; attempt < 48 && !stopToken.stop_requested(); ++attempt)
	{
		CloseTaskbarAutoHideConflictDialog();
		if (attempt == 4 || attempt == 16 || attempt == 32)
		{
			SetTaskbarAutoHide();
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(250));
	}
}

void MainAppWindow::RemoveHideTrayIconOverride()
{
	m_HideIconOverride = false;
	UpdateTrayVisibility(!m_App.GetConfigManager().GetConfig().HideTray.value_or(false));
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
