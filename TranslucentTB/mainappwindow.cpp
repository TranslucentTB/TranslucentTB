#include "mainappwindow.hpp"
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>

#include "taskdialogs/aboutdialog.hpp"
#include "constants.hpp"
#include "undoc/dynamicloader.hpp"
#include "../ProgramLog/log.hpp"
#include "../ProgramLog/error/win32.hpp"

void MainAppWindow::LoadStartupTask() try
{
	using namespace winrt::Windows::Foundation;
	using namespace winrt::Windows::ApplicationModel;
	using Collections::IVectorView;
	StartupTask::GetForCurrentPackageAsync().Completed([this](const IAsyncOperation<IVectorView<StartupTask>> &op, AsyncStatus)
	{
		try
		{
			auto result = op.GetResults().GetAt(0);

			const auto lock = m_TaskLock.lock_exclusive();
			m_StartupTask = std::move(result);
		}
		HresultErrorCatch(spdlog::level::warn, L"Failed to get first startup task.");
	});
}
HresultErrorCatch(spdlog::level::warn, L"Failed to load package startup tasks.");

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
			// The app can be closed anytime after processing this message. Save the settings.
			Save();
		}

		return 0;

	default:
		return TrayContextMenu::MessageHandler(uMsg, wParam, lParam);
	}
}

void MainAppWindow::RefreshMenu()
{
	// TODO AppearanceMenuRefresh(ID_GROUP_DESKTOP, m_Config.DesktopAppearance, m_Config.UseRegularAppearanceWhenPeeking, false);
	AppearanceMenuRefresh(ID_GROUP_VISIBLE, m_Config.VisibleWindowAppearance);
	AppearanceMenuRefresh(ID_GROUP_MAXIMISED, m_Config.MaximisedWindowAppearance);
	AppearanceMenuRefresh(ID_GROUP_START, m_Config.StartOpenedAppearance);
	AppearanceMenuRefresh(ID_GROUP_CORTANA, m_Config.CortanaOpenedAppearance);
	AppearanceMenuRefresh(ID_GROUP_TIMELINE, m_Config.TimelineOpenedAppearance);

	LogMenuRefresh();

	CheckItem(ID_DISABLESAVING, m_Config.DisableSaving);

	if (m_HasPackageIdentity)
	{
		AutostartMenuRefresh();
	}
}

void MainAppWindow::AppearanceMenuRefresh(uint16_t group, TaskbarAppearance &appearance, bool &b, bool controlsEnabled)
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

void MainAppWindow::LogMenuRefresh()
{
	bool ok = false;
	bool logsEnabled = false;
	bool hasFile = false;
	uint16_t text = IDS_OPENLOG_ERROR;
	unsigned int levelButton = 0;

	if (const auto sink = Log::GetSink())
	{
		const auto level = sink->level();

		levelButton = level + ID_RADIOS_LOG;
		hasFile = sink->opened();

		if (hasFile)
		{
			ok = true;
			logsEnabled = level != spdlog::level::off;
			text = IDS_OPENLOG_NORMAL;
		}
		else if (!hasFile && !sink->tried())
		{
			ok = true;
			logsEnabled = level != spdlog::level::off;
			text = IDS_OPENLOG_EMPTY;
		}
	}

	EnableItem(ID_SUBMENU_LOG, ok);
	CheckItem(ID_SUBMENU_LOG, logsEnabled);
	EnableItem(ID_OPENLOG, hasFile);
	SetText(ID_OPENLOG, text);
	CheckRadio(ID_LOG_TRACE, ID_LOG_OFF, levelButton);
}

void MainAppWindow::AutostartMenuRefresh()
{
	bool userModifiable = false;
	bool enabled = false;
	uint16_t text = IDS_AUTOSTART_NORMAL;

	if (m_HasPackageIdentity)
	{
		try
		{
			const auto guard = m_TaskLock.lock_shared();
			if (m_StartupTask)
			{
				switch (m_StartupTask.State())
				{
					using winrt::Windows::ApplicationModel::StartupTaskState;

				case StartupTaskState::Disabled:
					userModifiable = true;
					break;

				case StartupTaskState::DisabledByPolicy:
					text = IDS_AUTOSTART_DISABLED_GPEDIT;
					break;

				case StartupTaskState::DisabledByUser:
					text = IDS_AUTOSTART_DISABLED_TASKMGR;
					break;

				case StartupTaskState::Enabled:
					userModifiable = true;
					enabled = true;
					break;

				case StartupTaskState::EnabledByPolicy:
					enabled = true;
					text = IDS_AUTOSTART_ENABLED_GPEDIT;
					break;

				default:
					text = IDS_AUTOSTART_UNKNOWN;
					break;
				}
			}
			else
			{
				text = IDS_AUTOSTART_NULL;
			}
		}
		catch (const winrt::hresult_error &err)
		{
			HresultErrorHandle(err, spdlog::level::warn, L"Failed to get startup task state");
			text = IDS_AUTOSTART_ERROR;
		}
	}
	else
	{
		text = IDS_AUTOSTART_NO_PKG_IDENT;
	}

	CheckItem(ID_AUTOSTART, enabled);
	EnableItem(ID_AUTOSTART, userModifiable);
	SetText(ID_AUTOSTART, text);
}

void MainAppWindow::ClickHandler(unsigned int id)
{
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
			AppearanceForGroup(group).Accent = static_cast<ACCENT_STATE>(offset);
			AppearanceChanged();
			break;
		case ID_GROUP_LOG:
			m_Config.LogVerbosity = static_cast<spdlog::level::level_enum>(offset);
			VerbosityChanged();
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
				if (const auto sink = Log::GetSink(); sink && sink->opened())
				{
					HresultVerify(win32::EditFile(sink->file()), spdlog::level::err, L"Failed to open log file in Notepad");
				}
				break;
			case ID_EDITSETTINGS:
				HresultVerify(win32::EditFile(m_ConfigPath), spdlog::level::err, L"Failed to open configuration file.");
				break;
			case ID_RETURNTODEFAULTSETTINGS:
				m_Config = { };
				ConfigurationReloaded();
				break;
			case ID_DISABLESAVING:
				InvertBool(m_Config.DisableSaving);
				break;
			case ID_HIDETRAY:
				HideTrayHandler();
				break;
			case ID_DUMPWORKER:
				m_Worker.DumpState();
				break;
			case ID_RESETWORKER:
				m_Worker.ResetState();
				break;
			case ID_ABOUT:
				AboutDialog(m_HasPackageIdentity, hinstance()).Run();
				break;
			case ID_AUTOSTART:
				AutostartMenuHandler();
				break;
			case ID_TIPS:
				HresultVerify(win32::OpenLink(L"https://" APP_NAME ".github.io/tips"), spdlog::level::err, L"Failed to open tips & tricks link.");
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

TaskbarAppearance &MainAppWindow::AppearanceForGroup(uint16_t group)
{
	switch (group)
	{
	case ID_GROUP_DESKTOP: return m_Config.DesktopAppearance;
	case ID_GROUP_VISIBLE: return m_Config.VisibleWindowAppearance;
	case ID_GROUP_MAXIMISED: return m_Config.MaximisedWindowAppearance;
	case ID_GROUP_START: return m_Config.StartOpenedAppearance;
	case ID_GROUP_CORTANA: return m_Config.CortanaOpenedAppearance;
	case ID_GROUP_TIMELINE: return m_Config.TimelineOpenedAppearance;
	default:
		assert(false);
		__assume(0);
	}
}

void MainAppWindow::AppearanceMenuHandler(uint8_t offset, TaskbarAppearance &appearance, bool &b)
{
	if (offset == ID_OFFSET_ENABLED)
	{
		InvertBool(b);
		AppearanceChanged();
	}
	else if (offset == ID_OFFSET_COLOR)
	{
		// TODO: color
	}
}

void MainAppWindow::HideTrayHandler()
{
	if (MessageBoxEx(
			Window::NullWindow,
			L"This change is temporary and will be lost the next time the configuration is loaded.\n\n"
			L"To make this permanent, edit the configuration file using \"Advanced\" > \"Edit settings\".\n\n"
			L"Are you sure you want to proceed?",
			APP_NAME,
			MB_YESNO | MB_ICONINFORMATION | MB_SETFOREGROUND,
			MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL)
		) == IDYES)
	{
		Hide();
	}
}

void MainAppWindow::AutostartMenuHandler() try
{
	if (!m_HasPackageIdentity)
	{
		MessagePrint(spdlog::level::err, L"No package identity!");
		return;
	}

	const auto lock = m_TaskLock.lock_shared();

	if (!m_StartupTask)
	{
		MessagePrint(spdlog::level::err, L"Startup task is null.");
		return;
	}

	switch (m_StartupTask.State())
	{
		using winrt::Windows::ApplicationModel::StartupTaskState;
		using namespace winrt::Windows::Foundation;

	case StartupTaskState::Disabled:
		m_StartupTask.RequestEnableAsync().Completed([](const IAsyncOperation<StartupTaskState> &op, AsyncStatus)
		{
			try
			{
				const auto result = op.GetResults();
				if (result != StartupTaskState::Enabled &&
					result != StartupTaskState::EnabledByPolicy)
				{
					MessagePrint(spdlog::level::err, L"A request to enable the startup task did not result in it being enabled.");
				}
			}
			HresultErrorCatch(spdlog::level::err, L"Failed to enable startup task.")
		});
		break;

	case StartupTaskState::Enabled:
		m_StartupTask.Disable();
		break;

	default:
		MessagePrint(spdlog::level::err, L"Cannot change startup state because it is locked by external factors (for example Task Manager or Group Policy).");
		break;
	}
}
HresultErrorCatch(spdlog::level::err, L"Changing startup task state failed!")

void MainAppWindow::Exit(bool save)
{
	if (save)
	{
		Save();
	}

	PostQuitMessage(0);
}

void MainAppWindow::VerbosityChanged()
{
	Log::SetLevel(m_Config.LogVerbosity);
}

void MainAppWindow::AppearanceChanged()
{
	m_Worker.ConfigurationChanged();
}

void MainAppWindow::TrayIconChanged()
{
	if (m_Config.HideTray)
	{
		Hide();
	}
	else
	{
		Show();
	}
}

void MainAppWindow::ConfigurationReloaded()
{
	VerbosityChanged();
	TrayIconChanged();
	AppearanceChanged();
}

void MainAppWindow::WatcherCallback(void *context, DWORD, std::wstring_view fileName)
{
	if (fileName.empty() || win32::IsSameFilename(fileName, CONFIG_FILE))
	{
		const auto that = static_cast<MainAppWindow *>(context);
		that->m_Config = Config::Load(that->m_ConfigPath);
		that->ConfigurationReloaded();
	}
}

MainAppWindow::MainAppWindow(std::filesystem::path configPath, bool hasPackageIdentity, HINSTANCE hInstance) :
	TrayContextMenu(TRAY_GUID, TRAY_WINDOW, APP_NAME, MAKEINTRESOURCE(IDI_TRAYWHITEICON), MAKEINTRESOURCE(IDI_TRAYBLACKICON), MAKEINTRESOURCE(IDR_TRAY_MENU), hInstance),
	m_ConfigPath(std::move(configPath)),
	m_Config(Config::Load(m_ConfigPath)),
	// (ab)use of comma operator to load verbosity as early as possible
	m_Worker((VerbosityChanged(), m_Config), hInstance),
	m_Watcher(m_ConfigPath.parent_path(), false, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE, WatcherCallback, this),
	m_HasPackageIdentity(hasPackageIdentity),
	m_StartupTask(nullptr)
{
	if (m_HasPackageIdentity)
	{
		LoadStartupTask();
	}
	else
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

	// Shows the tray icon if not disabled
	TrayIconChanged();
}

WPARAM MainAppWindow::Run()
{
	while (true)
	{
		switch (MsgWaitForMultipleObjectsEx(0, nullptr, INFINITE, QS_ALLINPUT, MWMO_ALERTABLE | MWMO_INPUTAVAILABLE))
		{
		case WAIT_OBJECT_0:
			// TODO: pretranslate
			for (MSG msg; PeekMessage(&msg, 0, 0, 0, PM_REMOVE);)
			{
				if (msg.message != WM_QUIT)
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
				else
				{
					return msg.wParam;
				}
			}
			[[fallthrough]];
		case WAIT_IO_COMPLETION:
			continue;

		case WAIT_FAILED:
			LastErrorHandle(spdlog::level::critical, L"Failed to enter alertable wait state!");
		default:
			MessagePrint(spdlog::level::critical, L"MsgWaitForMultipleObjectsEx returned an unexpected value!");
		}
	}
}

void MainAppWindow::CloseRemote()
{
	Window::Find(TRAY_WINDOW, APP_NAME).send_message(WM_CLOSE);
}
