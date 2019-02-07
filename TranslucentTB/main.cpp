// Standard API
#include <functional>
#include <string>
#include <thread>
#include <unordered_map>

// Windows API
#include "arch.h"
#include <PathCch.h>
#include <ShlObj.h>

// Local stuff
#include "aboutdialog.hpp"
#include "autofree.hpp"
#include "autostart.hpp"
#include "blacklist.hpp"
#include "constants.hpp"
#include "config.hpp"
#include "folderwatcher.hpp"
#include "messagewindow.hpp"
#include "resource.h"
#include "swcadef.h"
#include "welcomedialog.hpp"
#include "taskbarattributeworker.hpp"
#include "traycontextmenu.hpp"
#include "ttberror.hpp"
#include "ttblog.hpp"
#include "util.hpp"
#include "uwp.hpp"
#include "win32.hpp"
#include "window.hpp"
#include "windowclass.hpp"

#pragma region Data

enum class EXITREASON {
	NewInstance,		// New instance told us to exit
	UserAction,			// Triggered by the user
	UserActionNoSave	// Triggered by the user, but doesn't saves config
};

static struct {
	EXITREASON exit_reason = EXITREASON::UserAction;
	std::wstring config_folder;
	std::wstring config_file;
	std::wstring exclude_file;
} run;

static const std::unordered_map<ACCENT_STATE, uint32_t> REGULAR_BUTTOM_MAP = {
	{ ACCENT_NORMAL,						ID_REGULAR_NORMAL },
	{ ACCENT_ENABLE_TRANSPARENTGRADIENT,	ID_REGULAR_CLEAR  },
	{ ACCENT_ENABLE_GRADIENT,				ID_REGULAR_OPAQUE },
	{ ACCENT_ENABLE_BLURBEHIND,				ID_REGULAR_BLUR   },
	{ ACCENT_ENABLE_ACRYLICBLURBEHIND,		ID_REGULAR_FLUENT }
};

static const std::unordered_map<ACCENT_STATE, uint32_t> MAXIMISED_BUTTON_MAP = {
	{ ACCENT_NORMAL,						ID_MAXIMISED_NORMAL },
	{ ACCENT_ENABLE_TRANSPARENTGRADIENT,	ID_MAXIMISED_CLEAR  },
	{ ACCENT_ENABLE_GRADIENT,				ID_MAXIMISED_OPAQUE },
	{ ACCENT_ENABLE_BLURBEHIND,				ID_MAXIMISED_BLUR   },
	{ ACCENT_ENABLE_ACRYLICBLURBEHIND,		ID_MAXIMISED_FLUENT }
};

static const std::unordered_map<ACCENT_STATE, uint32_t> START_BUTTON_MAP = {
	{ ACCENT_NORMAL,						ID_START_NORMAL },
	{ ACCENT_ENABLE_TRANSPARENTGRADIENT,	ID_START_CLEAR  },
	{ ACCENT_ENABLE_GRADIENT,				ID_START_OPAQUE },
	{ ACCENT_ENABLE_BLURBEHIND,				ID_START_BLUR   },
	{ ACCENT_ENABLE_ACRYLICBLURBEHIND,		ID_START_FLUENT }
};

static const std::unordered_map<ACCENT_STATE, uint32_t> CORTANA_BUTTON_MAP = {
	{ ACCENT_NORMAL,						ID_CORTANA_NORMAL },
	{ ACCENT_ENABLE_TRANSPARENTGRADIENT,	ID_CORTANA_CLEAR  },
	{ ACCENT_ENABLE_GRADIENT,				ID_CORTANA_OPAQUE },
	{ ACCENT_ENABLE_BLURBEHIND,				ID_CORTANA_BLUR   },
	{ ACCENT_ENABLE_ACRYLICBLURBEHIND,		ID_CORTANA_FLUENT }
};

static const std::unordered_map<ACCENT_STATE, uint32_t> TIMELINE_BUTTON_MAP = {
	{ ACCENT_NORMAL,						ID_TIMELINE_NORMAL },
	{ ACCENT_ENABLE_TRANSPARENTGRADIENT,	ID_TIMELINE_CLEAR  },
	{ ACCENT_ENABLE_GRADIENT,				ID_TIMELINE_OPAQUE },
	{ ACCENT_ENABLE_BLURBEHIND,				ID_TIMELINE_BLUR   },
	{ ACCENT_ENABLE_ACRYLICBLURBEHIND,		ID_TIMELINE_FLUENT }
};

static const std::unordered_map<enum Config::PEEK, uint32_t> PEEK_BUTTON_MAP = {
	{ Config::PEEK::Enabled,					ID_PEEK_SHOW    },
	{ Config::PEEK::DynamicMainMonitor,			ID_PEEK_DYNAMIC_MAIN_MONITOR },
	{ Config::PEEK::DynamicAnyMonitor,			ID_PEEK_DYNAMIC_ANY_MONITOR },
	{ Config::PEEK::DynamicDesktopForeground,	ID_PEEK_DYNAMIC_FOREGROUND_DESKTOP },
	{ Config::PEEK::Disabled,					ID_PEEK_HIDE    }
};

#pragma endregion

#pragma region Configuration

#if 0
void GetPaths()
{
	const wchar_t *appData;
	AutoFree::CoTaskMem<wchar_t[]> appDataSafe;
	std::wstring configFolderName;

	std::wstring exeFolder_str = win32::GetExeLocation();
	exeFolder_str.erase(exeFolder_str.find_last_of(LR"(/\)") + 1);

	AutoFree::Local<wchar_t[]> portableModeFile;
	if (ErrorHandle(PathAllocCombine(exeFolder_str.c_str(), L"portable", PATHCCH_ALLOW_LONG_PATHS, portableModeFile.put()), Error::Level::Error, L"Failed to combine executable folder and portable file!")
		&& win32::FileExists(portableModeFile.get()))
	{
		appData = exeFolder_str.c_str();
		configFolderName = L"config";
	}
	else
	{
		ErrorHandle(SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_DEFAULT, NULL, appDataSafe.put()), Error::Level::Fatal, L"Failed to determine configuration files locations!");
		appData = appDataSafe.get();
		configFolderName = NAME;
	}

	GetPaths_common(appData, configFolderName);
}
#endif

void GetPaths()
{
	try
	{
		AutoFree::Local<wchar_t[]> configFolder;
		AutoFree::Local<wchar_t[]> configFile;
		AutoFree::Local<wchar_t[]> excludeFile;

		ErrorHandle(PathAllocCombine(UWP::GetApplicationFolderPath(UWP::FolderType::Roaming).c_str(), NAME, PATHCCH_ALLOW_LONG_PATHS, configFolder.put()), Error::Level::Fatal, L"Failed to combine roaming folder and application name!");
		ErrorHandle(PathAllocCombine(configFolder.get(), CONFIG_FILE, PATHCCH_ALLOW_LONG_PATHS, configFile.put()), Error::Level::Fatal, L"Failed to combine config folder and config file!");
		ErrorHandle(PathAllocCombine(configFolder.get(), EXCLUDE_FILE, PATHCCH_ALLOW_LONG_PATHS, excludeFile.put()), Error::Level::Fatal, L"Failed to combine config folder and exclude file!");

		run.config_folder = configFolder.get();
		run.config_file = configFile.get();
		run.exclude_file = excludeFile.get();
	}
	catch (const winrt::hresult_error &error)
	{
		ErrorHandle(error.code(), Error::Level::Fatal, L"Getting application folder paths failed!");
	}
}

void ApplyStock(const std::wstring &filename)
{
	std::wstring exeFolder_str = win32::GetExeLocation();
	exeFolder_str.erase(exeFolder_str.find_last_of(LR"(/\)") + 1);

	AutoFree::Local<wchar_t[]> stockFile;
	if (!ErrorHandle(PathAllocCombine(exeFolder_str.c_str(), filename.c_str(), PATHCCH_ALLOW_LONG_PATHS, stockFile.put()), Error::Level::Error, L"Failed to combine executable folder and config file!"))
	{
		return;
	}

	AutoFree::Local<wchar_t[]> configFile;
	if (!ErrorHandle(PathAllocCombine(run.config_folder.c_str(), filename.c_str(), PATHCCH_ALLOW_LONG_PATHS, configFile.put()), Error::Level::Error, L"Failed to combine config folder and config file!"))
	{
		return;
	}

	if (!win32::IsDirectory(run.config_folder))
	{
		if (!CreateDirectory(run.config_folder.c_str(), NULL))
		{
			LastErrorHandle(Error::Level::Error, L"Creating configuration files directory failed!");
			return;
		}
	}

	if (!CopyFile(stockFile.get(), configFile.get(), FALSE))
	{
		LastErrorHandle(Error::Level::Error, L"Copying stock configuration file failed!");
	}
}

bool CheckAndRunWelcome()
{
	if (!win32::IsDirectory(run.config_folder))
	{
		if (!WelcomeDialog(run.config_folder).Run())
		{
			return false;
		}
	}
	if (!win32::FileExists(run.config_file))
	{
		ApplyStock(CONFIG_FILE);
	}
	if (!win32::FileExists(run.exclude_file))
	{
		ApplyStock(EXCLUDE_FILE);
	}
	return true;
}

long LoadConfig(...)
{
	Config::Parse(run.config_file);
	Blacklist::Parse(run.exclude_file);
	return 0;
}

#pragma endregion

#pragma region Tray

void RefreshAutostartMenu(HMENU menu, const winrt::Windows::Foundation::IAsyncOperation<Autostart::StartupState> &sender, ...)
{
	const auto state = sender.GetResults();

	TrayContextMenu::RefreshBool(ID_AUTOSTART, menu, !(state == Autostart::StartupState::DisabledByUser
		|| state == Autostart::StartupState::DisabledByPolicy
		|| state == Autostart::StartupState::EnabledByPolicy),
		TrayContextMenu::ControlsEnabled);

	TrayContextMenu::RefreshBool(ID_AUTOSTART, menu, state == Autostart::StartupState::Enabled
		|| state == Autostart::StartupState::EnabledByPolicy,
		TrayContextMenu::Toggle);

	std::wstring autostart_text;
	switch (state)
	{
	case Autostart::StartupState::DisabledByUser:
		autostart_text = L"Startup has been disabled in Task Manager";
		break;
	case Autostart::StartupState::DisabledByPolicy:
		autostart_text = L"Startup has been disabled in Group Policy";
		break;
	case Autostart::StartupState::EnabledByPolicy:
		autostart_text = L"Startup has been enabled in Group Policy";
		break;
	case Autostart::StartupState::Enabled:
	case Autostart::StartupState::Disabled:
		autostart_text = L"Open at boot";
	}
	TrayContextMenu::ChangeItemText(menu, ID_AUTOSTART, std::move(autostart_text));
}

void RefreshMenu(HMENU menu)
{
	TrayContextMenu::RefreshBool(ID_AUTOSTART, menu, false, TrayContextMenu::ControlsEnabled);
	TrayContextMenu::RefreshBool(ID_AUTOSTART, menu, false, TrayContextMenu::Toggle);
	TrayContextMenu::ChangeItemText(menu, ID_AUTOSTART, L"Querying startup state...");
	Autostart::GetStartupState().Completed(std::bind(&RefreshAutostartMenu, menu, std::placeholders::_1));


	const bool has_log = !Log::file().empty();
	TrayContextMenu::RefreshBool(ID_OPENLOG, menu, has_log, TrayContextMenu::ControlsEnabled);
	TrayContextMenu::ChangeItemText(menu, ID_OPENLOG, has_log
		? L"Open log file"
		: Log::init_done()
			? L"Error when initializing log file"
			: L"Nothing has been logged yet"
	);

	TrayContextMenu::RefreshBool(ID_SAVESETTINGS, menu, !Config::NO_SAVE, TrayContextMenu::ControlsEnabled);

	TrayContextMenu::RefreshBool(ID_REGULAR_COLOR,   menu,
		Config::REGULAR_APPEARANCE.ACCENT != ACCENT_NORMAL,
		TrayContextMenu::ControlsEnabled);
	TrayContextMenu::RefreshBool(ID_MAXIMISED_COLOR, menu,
		Config::MAXIMISED_ENABLED && Config::MAXIMISED_APPEARANCE.ACCENT != ACCENT_NORMAL,
		TrayContextMenu::ControlsEnabled);
	TrayContextMenu::RefreshBool(ID_START_COLOR,     menu,
		Config::START_ENABLED     && Config::START_APPEARANCE.ACCENT != ACCENT_NORMAL,
		TrayContextMenu::ControlsEnabled);
	TrayContextMenu::RefreshBool(ID_CORTANA_COLOR,     menu,
		Config::CORTANA_ENABLED   && Config::CORTANA_APPEARANCE.ACCENT != ACCENT_NORMAL,
		TrayContextMenu::ControlsEnabled);
	TrayContextMenu::RefreshBool(ID_TIMELINE_COLOR,  menu,
		Config::TIMELINE_ENABLED  && Config::TIMELINE_APPEARANCE.ACCENT != ACCENT_NORMAL,
		TrayContextMenu::ControlsEnabled);
}

long ExitApp(EXITREASON reason, ...)
{
	run.exit_reason = reason;
	PostQuitMessage(0);
	return 0;
}

#pragma endregion

#pragma region Main logic

// todo: move to worker
//void SetTaskbarBlur()
//{
//	static uint8_t counter = 10;
//
//	if (counter >= 10)	// Change this if you want to change the time it takes for the program to update.
//	{					// 1 = Config::SLEEP_TIME; we use 10 (assuming the default configuration value of 10),
//						// because the difference is less noticeable and it has no large impact on CPU.
//						// We can change this if we feel that CPU is more important than response time.
//		run.should_show_peek = (Config::PEEK == Config::PEEK::Enabled);
//
//		for (auto &[_, pair] : run.taskbars)
//		{
//			pair.second = &Config::REGULAR_APPEARANCE; // Reset taskbar state
//		}
//		if (Config::MAXIMISED_ENABLED || Config::PEEK == Config::PEEK::Dynamic)
//		{
//			EnumWindows(&EnumWindowsProcess, NULL);
//		}
//
//		TogglePeek(run.should_show_peek);
//
//		const Window fg_window = Window::ForegroundWindow();
//		if (fg_window != Window::NullWindow && run.taskbars.count(fg_window.monitor()) != 0)
//		{
//			if (Config::CORTANA_ENABLED && !run.start_opened && !fg_window.get_attribute<BOOL>(DWMWA_CLOAKED) &&
//				Util::IgnoreCaseStringEquals(*fg_window.filename(), L"SearchUI.exe"))
//			{
//				run.taskbars.at(fg_window.monitor()).second = &Config::CORTANA_APPEARANCE;
//			}
//
//			if (Config::START_ENABLED && run.start_opened)
//			{
//				run.taskbars.at(fg_window.monitor()).second = &Config::START_APPEARANCE;
//			}
//		}
//
//		// Put this between Start/Cortana and Task view/Timeline
//		// Task view and Timeline show over Aero Peek, but not Start or Cortana
//		if (Config::MAXIMISED_ENABLED && Config::MAXIMISED_REGULAR_ON_PEEK && run.peek_active)
//		{
//			for (auto &[_, pair] : run.taskbars)
//			{
//				pair.second = &Config::REGULAR_APPEARANCE;
//			}
//		}
//
//		if (fg_window != Window::NullWindow)
//		{
//			static const bool timeline_av = win32::IsAtLeastBuild(MIN_FLUENT_BUILD);
//			if (Config::TIMELINE_ENABLED && (timeline_av
//				? (*fg_window.classname() == CORE_WINDOW && Util::IgnoreCaseStringEquals(*fg_window.filename(), L"Explorer.exe"))
//				: (*fg_window.classname() == L"MultitaskingViewFrame")))
//			{
//				for (auto &[_, pair] : run.taskbars)
//				{
//					pair.second = &Config::TIMELINE_APPEARANCE;
//				}
//			}
//		}
//
//		counter = 0;
//	}
//	else
//	{
//		counter++;
//	}
//
//	for (const auto &[_, pair] : run.taskbars)
//	{
//		const auto &[window, appearance] = pair;
//		SetWindowBlur(window, appearance->ACCENT, appearance->COLOR);
//	}
//}

#pragma endregion

#pragma region Startup

bool IsSingleInstance()
{
	static winrt::handle mutex;

	if (!mutex)
	{
		mutex.attach(CreateMutex(NULL, FALSE, MUTEX_GUID));
		DWORD error = GetLastError();
		switch (error)
		{
		case ERROR_ALREADY_EXISTS:
			return false;

		case ERROR_SUCCESS:
			return true;

		default:
			ErrorHandle(HRESULT_FROM_WIN32(error), Error::Level::Error, L"Failed to open app mutex!");
			return true;
		}
	}
	else
	{
		return true;
	}
}

void InitializeTray(HINSTANCE hInstance)
{
	static MessageWindow window(TRAY_WINDOW, NAME, hInstance);
	static TaskbarAttributeWorker worker(hInstance);
	static FolderWatcher watcher(run.config_folder, FILE_NOTIFY_CHANGE_LAST_WRITE, window);

	window.RegisterCallback(WM_FILECHANGED, LoadConfig);

	window.RegisterCallback(WM_NEWTTBINSTANCE, std::bind(&ExitApp, EXITREASON::NewInstance));

	window.RegisterCallback(WM_CLOSE, std::bind(&ExitApp, EXITREASON::UserAction));

	window.RegisterCallback(WM_QUERYENDSESSION, [](WPARAM, LPARAM lParam)
	{
		if (lParam & ENDSESSION_CLOSEAPP)
		{
			// The app is being queried if it can close for an update.
			RegisterApplicationRestart(NULL, NULL);
		}
		return TRUE;
	});

	window.RegisterCallback(WM_ENDSESSION, [](WPARAM wParam, ...)
	{
		if (wParam)
		{
			// The app can be closed anytime after processing this message. Save the settings.
			Config::Save(run.config_file);
		}

		return 0;
	});


	if (!Config::NO_TRAY)
	{
		static TrayContextMenu tray(window, MAKEINTRESOURCE(IDI_TRAYWHITEICON), MAKEINTRESOURCE(IDI_TRAYBLACKICON), MAKEINTRESOURCE(IDR_TRAY_MENU), hInstance);

		tray.BindColor(ID_REGULAR_COLOR, Config::REGULAR_APPEARANCE.COLOR);
		tray.BindEnum(Config::REGULAR_APPEARANCE.ACCENT, REGULAR_BUTTOM_MAP);


		tray.BindBool(ID_MAXIMISED,      Config::MAXIMISED_ENABLED,         TrayContextMenu::Toggle);
		tray.BindBool(ID_MAXIMISED_PEEK, Config::MAXIMISED_ENABLED,         TrayContextMenu::ControlsEnabled);
		tray.BindBool(ID_MAXIMISED_PEEK, Config::MAXIMISED_REGULAR_ON_PEEK, TrayContextMenu::Toggle);
		tray.BindColor(ID_MAXIMISED_COLOR, Config::MAXIMISED_APPEARANCE.COLOR);
		tray.BindEnum(Config::MAXIMISED_APPEARANCE.ACCENT, MAXIMISED_BUTTON_MAP);
		for (const auto &[_, id] : MAXIMISED_BUTTON_MAP)
		{
			tray.BindBool(id, Config::MAXIMISED_ENABLED, TrayContextMenu::ControlsEnabled);
		}


		tray.BindBool(ID_START, Config::START_ENABLED, TrayContextMenu::Toggle);
		tray.BindColor(ID_START_COLOR, Config::START_APPEARANCE.COLOR);
		tray.BindEnum(Config::START_APPEARANCE.ACCENT, START_BUTTON_MAP);
		for (const auto &[_, id] : START_BUTTON_MAP)
		{
			tray.BindBool(id, Config::START_ENABLED, TrayContextMenu::ControlsEnabled);
		}


		tray.BindBool(ID_CORTANA, Config::CORTANA_ENABLED, TrayContextMenu::Toggle);
		tray.BindColor(ID_CORTANA_COLOR, Config::CORTANA_APPEARANCE.COLOR);
		tray.BindEnum(Config::CORTANA_APPEARANCE.ACCENT, CORTANA_BUTTON_MAP);
		for (const auto &[_, id] : CORTANA_BUTTON_MAP)
		{
			tray.BindBool(id, Config::CORTANA_ENABLED, TrayContextMenu::ControlsEnabled);
		}


		tray.BindBool(ID_TIMELINE, Config::TIMELINE_ENABLED, TrayContextMenu::Toggle);
		tray.BindColor(ID_TIMELINE_COLOR, Config::TIMELINE_APPEARANCE.COLOR);
		tray.BindEnum(Config::TIMELINE_APPEARANCE.ACCENT, TIMELINE_BUTTON_MAP);
		for (const auto &[_, id] : TIMELINE_BUTTON_MAP)
		{
			tray.BindBool(id, Config::TIMELINE_ENABLED, TrayContextMenu::ControlsEnabled);
		}


		tray.BindEnum(Config::PEEK, PEEK_BUTTON_MAP);


		tray.RegisterContextMenuCallback(ID_OPENLOG, []
		{
			Log::Flush();
			win32::EditFile(Log::file());
		});
		tray.BindBool(ID_VERBOSE, Config::VERBOSE, TrayContextMenu::Toggle);
		tray.RegisterContextMenuCallback(ID_SAVESETTINGS, []
		{
			Config::Save(run.config_file);
			std::thread(std::bind(&MessageBox, Window::NullWindow, L"Settings have been saved.", NAME, MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND)).detach();
		});
		tray.RegisterContextMenuCallback(ID_RELOADSETTINGS, std::bind(&Config::Parse, std::ref(run.config_file)));
		tray.RegisterContextMenuCallback(ID_EDITSETTINGS, []
		{
			Config::Save(run.config_file);
			win32::EditFile(run.config_file);
		});
		tray.RegisterContextMenuCallback(ID_RETURNTODEFAULTSETTINGS, []
		{
			ApplyStock(CONFIG_FILE);
		});
		tray.RegisterContextMenuCallback(ID_RELOADDYNAMICBLACKLIST, std::bind(&Blacklist::Parse, std::ref(run.exclude_file)));
		tray.RegisterContextMenuCallback(ID_EDITDYNAMICBLACKLIST, std::bind(&win32::EditFile, std::ref(run.exclude_file)));
		tray.RegisterContextMenuCallback(ID_RETURNTODEFAULTBLACKLIST, []
		{
			ApplyStock(EXCLUDE_FILE);
		});
		tray.RegisterContextMenuCallback(ID_CLEARWINDOWCACHE, Window::ClearCache);
		tray.RegisterContextMenuCallback(ID_CLEARBLACKLISTCACHE, Blacklist::ClearCache);
		tray.RegisterContextMenuCallback(ID_RESETWORKER, std::bind(&TaskbarAttributeWorker::ResetState, &worker));
		tray.RegisterContextMenuCallback(ID_ABOUT, []
		{
			std::thread([]
			{
				AboutDialog().Run();
			}).detach();
		});
		tray.RegisterContextMenuCallback(ID_EXITWITHOUTSAVING, std::bind(&ExitApp, EXITREASON::UserActionNoSave));


		tray.RegisterContextMenuCallback(ID_AUTOSTART, []
		{
			Autostart::GetStartupState().Completed([](auto &&sender, ...)
			{
				Autostart::SetStartupState(sender.GetResults() == Autostart::StartupState::Enabled ? Autostart::StartupState::Disabled : Autostart::StartupState::Enabled);
			});
		});
		tray.RegisterContextMenuCallback(ID_TIPS, std::bind(&win32::OpenLink,
			L"https://github.com/TranslucentTB/TranslucentTB/wiki/Tips-and-tricks-for-a-better-looking-taskbar"));
		tray.RegisterContextMenuCallback(ID_EXIT, std::bind(&ExitApp, EXITREASON::UserAction));


		tray.RegisterCustomRefresh(RefreshMenu);
	}
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, wchar_t *, int)
{
	win32::HardenProcess();
	try
	{
		winrt::init_apartment(winrt::apartment_type::single_threaded);
	}
	catch (const winrt::hresult_error &error)
	{
		ErrorHandle(error.code(), Error::Level::Fatal, L"Initialization of Windows Runtime failed.");
	}

	// If there already is another instance running, tell it to exit
	if (!IsSingleInstance())
	{
		Window::Find(TRAY_WINDOW, NAME).send_message(WM_NEWTTBINSTANCE);
	}

	// Get configuration file paths
	GetPaths();

	// If the configuration files don't exist, restore the files and show welcome to the users
	if (!CheckAndRunWelcome())
	{
		return EXIT_FAILURE;
	}

	// Parse our configuration
	LoadConfig();
	if (!Config::ParseCommandLine())
	{
		return EXIT_SUCCESS;
	}

	// Initialize GUI
	InitializeTray(hInstance);

	// Run the main program loop. When this method exits, TranslucentTB itself is about to exit.
	MessageWindow::RunMessageLoop();

	// If it's a new instance, don't save or restore taskbar to default
	if (run.exit_reason != EXITREASON::NewInstance)
	{
		if (run.exit_reason != EXITREASON::UserActionNoSave)
		{
			Config::Save(run.config_file);
		}
	}

	return EXIT_SUCCESS;
}

#pragma endregion