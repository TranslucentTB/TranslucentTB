// Standard API
#include <chrono>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>

// Windows API
#include "arch.h"
#include <PathCch.h>
#include <ShlObj.h>

// Local stuff
#include "aboutdialog.hpp"
#include "appvisibilitysink.hpp"
#include "autofree.hpp"
#include "autostart.hpp"
#include "blacklist.hpp"
#include "common.hpp"
#include "config.hpp"
#include "createinstance.hpp"
#include "eventhook.hpp"
#include "folderwatcher.hpp"
#include "hook.hpp"
#include "../ExplorerDetour/hook.hpp"
#include "messagewindow.hpp"
#include "resource.h"
#include "swcadata.hpp"
#include "welcomedialog.hpp"
#include "taskbarattributeworker.hpp"
#include "traycontextmenu.hpp"
#include "ttberror.hpp"
#include "ttblog.hpp"
#include "util.hpp"
#ifdef STORE
#include "uwp.hpp"
#endif
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
	Window main_taskbar;
	std::unordered_map<HMONITOR, std::pair<Window, const Config::TASKBAR_APPEARANCE *>> taskbars;
	std::vector<TTBHook> hooks;
	bool hooks_disabled = false;
	bool should_show_peek = true;
	bool is_running = true;
	std::wstring config_folder;
	std::wstring config_file;
	std::wstring exclude_file;
	bool peek_active = false;
	bool start_opened = false;
} run;

static const std::unordered_map<swca::ACCENT, uint32_t> REGULAR_BUTTOM_MAP = {
	{ swca::ACCENT::ACCENT_NORMAL,						IDM_REGULAR_NORMAL },
	{ swca::ACCENT::ACCENT_ENABLE_TRANSPARENTGRADIENT,	IDM_REGULAR_CLEAR  },
	{ swca::ACCENT::ACCENT_ENABLE_GRADIENT,				IDM_REGULAR_OPAQUE },
	{ swca::ACCENT::ACCENT_ENABLE_BLURBEHIND,			IDM_REGULAR_BLUR   },
	{ swca::ACCENT::ACCENT_ENABLE_FLUENT,				IDM_REGULAR_FLUENT }
};

static const std::unordered_map<swca::ACCENT, uint32_t> MAXIMISED_BUTTON_MAP = {
	{ swca::ACCENT::ACCENT_NORMAL,						IDM_MAXIMISED_NORMAL },
	{ swca::ACCENT::ACCENT_ENABLE_TRANSPARENTGRADIENT,	IDM_MAXIMISED_CLEAR  },
	{ swca::ACCENT::ACCENT_ENABLE_GRADIENT,				IDM_MAXIMISED_OPAQUE },
	{ swca::ACCENT::ACCENT_ENABLE_BLURBEHIND,			IDM_MAXIMISED_BLUR   },
	{ swca::ACCENT::ACCENT_ENABLE_FLUENT,				IDM_MAXIMISED_FLUENT }
};

static const std::unordered_map<swca::ACCENT, uint32_t> START_BUTTON_MAP = {
	{ swca::ACCENT::ACCENT_NORMAL,						IDM_START_NORMAL },
	{ swca::ACCENT::ACCENT_ENABLE_TRANSPARENTGRADIENT,	IDM_START_CLEAR  },
	{ swca::ACCENT::ACCENT_ENABLE_GRADIENT,				IDM_START_OPAQUE },
	{ swca::ACCENT::ACCENT_ENABLE_BLURBEHIND,			IDM_START_BLUR   },
	{ swca::ACCENT::ACCENT_ENABLE_FLUENT,				IDM_START_FLUENT }
};

static const std::unordered_map<swca::ACCENT, uint32_t> CORTANA_BUTTON_MAP = {
	{ swca::ACCENT::ACCENT_NORMAL,						IDM_CORTANA_NORMAL },
	{ swca::ACCENT::ACCENT_ENABLE_TRANSPARENTGRADIENT,	IDM_CORTANA_CLEAR  },
	{ swca::ACCENT::ACCENT_ENABLE_GRADIENT,				IDM_CORTANA_OPAQUE },
	{ swca::ACCENT::ACCENT_ENABLE_BLURBEHIND,			IDM_CORTANA_BLUR   },
	{ swca::ACCENT::ACCENT_ENABLE_FLUENT,				IDM_CORTANA_FLUENT }
};

static const std::unordered_map<swca::ACCENT, uint32_t> TIMELINE_BUTTON_MAP = {
	{ swca::ACCENT::ACCENT_NORMAL,						IDM_TIMELINE_NORMAL },
	{ swca::ACCENT::ACCENT_ENABLE_TRANSPARENTGRADIENT,	IDM_TIMELINE_CLEAR  },
	{ swca::ACCENT::ACCENT_ENABLE_GRADIENT,				IDM_TIMELINE_OPAQUE },
	{ swca::ACCENT::ACCENT_ENABLE_BLURBEHIND,			IDM_TIMELINE_BLUR   },
	{ swca::ACCENT::ACCENT_ENABLE_FLUENT,				IDM_TIMELINE_FLUENT }
};

static const std::unordered_map<enum Config::PEEK, uint32_t> PEEK_BUTTON_MAP = {
	{ Config::PEEK::Enabled,		IDM_PEEK_SHOW    },
	{ Config::PEEK::Dynamic,		IDM_PEEK_DYNAMIC },
	{ Config::PEEK::Disabled,		IDM_PEEK_HIDE    }
};

#pragma endregion

#pragma region Configuration

void GetPaths_common(const wchar_t *appData, const std::wstring &cfgFolder)
{
	AutoFree::Local<wchar_t[]> configFolder;
	AutoFree::Local<wchar_t[]> configFile;
	AutoFree::Local<wchar_t[]> excludeFile;

	ErrorHandle(PathAllocCombine(appData, cfgFolder.c_str(), PATHCCH_ALLOW_LONG_PATHS, configFolder.put()), Error::Level::Fatal, L"Failed to combine AppData folder and application name!");
	ErrorHandle(PathAllocCombine(configFolder.get(), CONFIG_FILE, PATHCCH_ALLOW_LONG_PATHS, configFile.put()), Error::Level::Fatal, L"Failed to combine config folder and config file!");
	ErrorHandle(PathAllocCombine(configFolder.get(), EXCLUDE_FILE, PATHCCH_ALLOW_LONG_PATHS, excludeFile.put()), Error::Level::Fatal, L"Failed to combine config folder and exclude file!");

	run.config_folder = configFolder.get();
	run.config_file = configFile.get();
	run.exclude_file = excludeFile.get();
}

#ifndef STORE
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
#else
void GetPaths()
{
	try
	{
		GetPaths_common(UWP::GetApplicationFolderPath(UWP::FolderType::Roaming).c_str(), NAME);
	}
	catch (const winrt::hresult_error &error)
	{
		ErrorHandle(error.code(), Error::Level::Fatal, L"Getting application folder paths failed!");
	}
}
#endif

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

void LoadConfig()
{
	Config::Parse(run.config_file);
	Blacklist::Parse(run.exclude_file);
}

#pragma endregion

#pragma region Utilities

// todo: move to worker?
void TogglePeek(const bool &status)
{
	static bool cached_peek = true;
	static Window cached_taskbar = Window(run.main_taskbar);

	if (status != cached_peek || cached_taskbar != run.main_taskbar)
	{
		Window _peek = Window::Find(L"TrayShowDesktopButtonWClass", L"", Window::Find(L"TrayNotifyWnd", L"", run.main_taskbar));

		if (!status)
		{
			SetWindowLong(_peek, GWL_EXSTYLE, GetWindowLong(_peek, GWL_EXSTYLE) | WS_EX_LAYERED);

			SetLayeredWindowAttributes(_peek, 0, 0, LWA_ALPHA);
		}
		else
		{
			SetWindowLong(_peek, GWL_EXSTYLE, GetWindowLong(_peek, GWL_EXSTYLE) & ~WS_EX_LAYERED);
		}

		cached_peek = status;
		cached_taskbar = Window(run.main_taskbar);
	}
}

#pragma endregion

#pragma region Tray

void RefreshAutostartMenu(HMENU menu, const Autostart::StartupState &state)
{
	TrayContextMenu::RefreshBool(IDM_AUTOSTART, menu, !(state == Autostart::StartupState::DisabledByUser
		|| state == Autostart::StartupState::DisabledByPolicy
		|| state == Autostart::StartupState::EnabledByPolicy),
		TrayContextMenu::ControlsEnabled);

	TrayContextMenu::RefreshBool(IDM_AUTOSTART, menu, state == Autostart::StartupState::Enabled
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
	TrayContextMenu::ChangeItemText(menu, IDM_AUTOSTART, std::move(autostart_text));
}

void RefreshMenu(HMENU menu)
{
	TrayContextMenu::RefreshBool(IDM_AUTOSTART, menu, false, TrayContextMenu::ControlsEnabled);
	TrayContextMenu::RefreshBool(IDM_AUTOSTART, menu, false, TrayContextMenu::Toggle);
	TrayContextMenu::ChangeItemText(menu, IDM_AUTOSTART, L"Querying startup state...");
	Autostart::GetStartupState().then(std::bind(&RefreshAutostartMenu, menu, std::placeholders::_1));


	static bool initial_check_done = false;
	if (!initial_check_done)
	{
		if (!win32::IsAtLeastBuild(MIN_FLUENT_BUILD))
		{
			RemoveMenu(menu, IDM_REGULAR_FLUENT,   MF_BYCOMMAND);
			RemoveMenu(menu, IDM_MAXIMISED_FLUENT, MF_BYCOMMAND);
			RemoveMenu(menu, IDM_START_FLUENT,     MF_BYCOMMAND);
			RemoveMenu(menu, IDM_CORTANA_FLUENT,   MF_BYCOMMAND);
			RemoveMenu(menu, IDM_TIMELINE_FLUENT,  MF_BYCOMMAND);

			// Same build for Timeline and fluent
			TrayContextMenu::ChangeItemText(menu, IDM_TIMELINE_POPUP, L"Task View opened");
		}

		initial_check_done = true;
	}

	const bool has_log = !Log::file().empty();
	TrayContextMenu::RefreshBool(IDM_OPENLOG, menu, has_log, TrayContextMenu::ControlsEnabled);
	TrayContextMenu::ChangeItemText(menu, IDM_OPENLOG, has_log
		? L"Open log file"
		: Log::init_done()
			? L"Error when initializing log file"
			: L"Nothing has been logged yet"
	);

	TrayContextMenu::RefreshBool(IDM_SAVESETTINGS, menu, !Config::NO_SAVE, TrayContextMenu::ControlsEnabled);

	TrayContextMenu::RefreshBool(IDM_REGULAR_COLOR,   menu,
		Config::REGULAR_APPEARANCE.ACCENT != swca::ACCENT::ACCENT_NORMAL,
		TrayContextMenu::ControlsEnabled);
	TrayContextMenu::RefreshBool(IDM_MAXIMISED_COLOR, menu,
		Config::MAXIMISED_ENABLED && Config::MAXIMISED_APPEARANCE.ACCENT != swca::ACCENT::ACCENT_NORMAL,
		TrayContextMenu::ControlsEnabled);
	TrayContextMenu::RefreshBool(IDM_START_COLOR,     menu,
		Config::START_ENABLED     && Config::START_APPEARANCE.ACCENT != swca::ACCENT::ACCENT_NORMAL,
		TrayContextMenu::ControlsEnabled);
	TrayContextMenu::RefreshBool(IDM_CORTANA_COLOR,     menu,
		Config::CORTANA_ENABLED   && Config::CORTANA_APPEARANCE.ACCENT != swca::ACCENT::ACCENT_NORMAL,
		TrayContextMenu::ControlsEnabled);
	TrayContextMenu::RefreshBool(IDM_TIMELINE_COLOR,  menu,
		Config::TIMELINE_ENABLED  && Config::TIMELINE_APPEARANCE.ACCENT != swca::ACCENT::ACCENT_NORMAL,
		TrayContextMenu::ControlsEnabled);
	TrayContextMenu::RefreshBool(IDM_PEEK_ONLY_MAIN,  menu,
		Config::PEEK == Config::PEEK::Dynamic,
		TrayContextMenu::ControlsEnabled);
}

long ExitApp(const EXITREASON &reason, ...)
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

void InitializeTray(const HINSTANCE &hInstance)
{
	static MessageWindow window(TRAY_WINDOW, NAME, hInstance);

	window.RegisterCallback(NEW_TTB_INSTANCE, std::bind(&ExitApp, EXITREASON::NewInstance));

	window.RegisterCallback(WM_CLOSE, std::bind(&ExitApp, EXITREASON::UserAction));

	window.RegisterCallback(WM_QUERYENDSESSION, [](WPARAM, const LPARAM lParam)
	{
		if (lParam & ENDSESSION_CLOSEAPP)
		{
			// The app is being queried if it can close for an update.
			RegisterApplicationRestart(NULL, NULL);
		}
		return TRUE;
	});

	window.RegisterCallback(WM_ENDSESSION, [](const WPARAM wParam, ...)
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
		static TrayContextMenu tray(window, MAKEINTRESOURCE(TRAYICON), MAKEINTRESOURCE(IDR_POPUP_MENU), hInstance);

		tray.BindColor(IDM_REGULAR_COLOR, Config::REGULAR_APPEARANCE.COLOR);
		tray.BindEnum(Config::REGULAR_APPEARANCE.ACCENT, REGULAR_BUTTOM_MAP);


		tray.BindBool(IDM_MAXIMISED,      Config::MAXIMISED_ENABLED,         TrayContextMenu::Toggle);
		tray.BindBool(IDM_MAXIMISED_PEEK, Config::MAXIMISED_ENABLED,         TrayContextMenu::ControlsEnabled);
		tray.BindBool(IDM_MAXIMISED_PEEK, Config::MAXIMISED_REGULAR_ON_PEEK, TrayContextMenu::Toggle);
		tray.BindColor(IDM_MAXIMISED_COLOR, Config::MAXIMISED_APPEARANCE.COLOR);
		tray.BindEnum(Config::MAXIMISED_APPEARANCE.ACCENT, MAXIMISED_BUTTON_MAP);
		for (const auto &[_, id] : MAXIMISED_BUTTON_MAP)
		{
			tray.BindBool(id, Config::MAXIMISED_ENABLED, TrayContextMenu::ControlsEnabled);
		}


		tray.BindBool(IDM_START, Config::START_ENABLED, TrayContextMenu::Toggle);
		tray.BindColor(IDM_START_COLOR, Config::START_APPEARANCE.COLOR);
		tray.BindEnum(Config::START_APPEARANCE.ACCENT, START_BUTTON_MAP);
		for (const auto &[_, id] : START_BUTTON_MAP)
		{
			tray.BindBool(id, Config::START_ENABLED, TrayContextMenu::ControlsEnabled);
		}

		tray.BindBool(IDM_CORTANA, Config::CORTANA_ENABLED, TrayContextMenu::Toggle);
		tray.BindColor(IDM_CORTANA_COLOR, Config::CORTANA_APPEARANCE.COLOR);
		tray.BindEnum(Config::CORTANA_APPEARANCE.ACCENT, CORTANA_BUTTON_MAP);
		for (const auto &[_, id] : CORTANA_BUTTON_MAP)
		{
			tray.BindBool(id, Config::CORTANA_ENABLED, TrayContextMenu::ControlsEnabled);
		}


		tray.BindBool(IDM_TIMELINE, Config::TIMELINE_ENABLED, TrayContextMenu::Toggle);
		tray.BindColor(IDM_TIMELINE_COLOR, Config::TIMELINE_APPEARANCE.COLOR);
		tray.BindEnum(Config::TIMELINE_APPEARANCE.ACCENT, TIMELINE_BUTTON_MAP);
		for (const auto &[_, id] : TIMELINE_BUTTON_MAP)
		{
			tray.BindBool(id, Config::TIMELINE_ENABLED, TrayContextMenu::ControlsEnabled);
		}


		tray.BindEnum(Config::PEEK, PEEK_BUTTON_MAP);
		tray.BindBool(IDM_PEEK_ONLY_MAIN, Config::PEEK_ONLY_MAIN, TrayContextMenu::Toggle);


		tray.RegisterContextMenuCallback(IDM_OPENLOG, []
		{
			Log::Flush();
			win32::EditFile(Log::file());
		});
		tray.BindBool(IDM_VERBOSE, Config::VERBOSE, TrayContextMenu::Toggle);
		tray.RegisterContextMenuCallback(IDM_SAVESETTINGS, []
		{
			Config::Save(run.config_file);
			std::thread(std::bind(&MessageBox, Window::NullWindow, L"Settings have been saved.", NAME, MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND)).detach();
		});
		tray.RegisterContextMenuCallback(IDM_RELOADSETTINGS, std::bind(&Config::Parse, std::ref(run.config_file)));
		tray.RegisterContextMenuCallback(IDM_EDITSETTINGS, []
		{
			Config::Save(run.config_file);
			win32::EditFile(run.config_file);
		});
		tray.RegisterContextMenuCallback(IDM_RETURNTODEFAULTSETTINGS, []
		{
			ApplyStock(CONFIG_FILE);
		});
		tray.RegisterContextMenuCallback(IDM_RELOADDYNAMICBLACKLIST, std::bind(&Blacklist::Parse, std::ref(run.exclude_file)));
		tray.RegisterContextMenuCallback(IDM_EDITDYNAMICBLACKLIST, std::bind(&win32::EditFile, std::ref(run.exclude_file)));
		tray.RegisterContextMenuCallback(IDM_RETURNTODEFAULTBLACKLIST, []
		{
			ApplyStock(EXCLUDE_FILE);
		});
		tray.RegisterContextMenuCallback(IDM_CLEARBLACKLISTCACHE, Blacklist::ClearCache);
		tray.RegisterContextMenuCallback(IDM_ABOUT, []
		{
			std::thread([]
			{
				AboutDialog().Run();
			}).detach();
		});
		tray.RegisterContextMenuCallback(IDM_EXITWITHOUTSAVING, std::bind(&ExitApp, EXITREASON::UserActionNoSave));


		tray.RegisterContextMenuCallback(IDM_AUTOSTART, []
		{
			Autostart::GetStartupState().then([](const Autostart::StartupState &result)
			{
				Autostart::SetStartupState(result == Autostart::StartupState::Enabled ? Autostart::StartupState::Disabled : Autostart::StartupState::Enabled);
			});
		});
		tray.RegisterContextMenuCallback(IDM_TIPS, std::bind(&win32::OpenLink,
			L"https://github.com/TranslucentTB/TranslucentTB/wiki/Tips-and-tricks-for-a-better-looking-taskbar"));
		tray.RegisterContextMenuCallback(IDM_EXIT, std::bind(&ExitApp, EXITREASON::UserAction));


		tray.RegisterCustomRefresh(RefreshMenu);
	}
}

int WINAPI wWinMain(const HINSTANCE hInstance, HINSTANCE, wchar_t *, int)
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
	if (!win32::IsSingleInstance())
	{
		Window::Find(TRAY_WINDOW, NAME).send_message(NEW_TTB_INSTANCE);
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

	// Watch for changes
	FolderWatcher config_watcher(run.config_folder, FILE_NOTIFY_CHANGE_LAST_WRITE, LoadConfig);

	// Initialize GUI
	InitializeTray(hInstance);

	TaskbarAttributeWorker worker(hInstance);

	// Undoc'd, allows to detect when Aero Peek starts and stops
	// todo: move to worker
	EventHook peek_hook(
		0x21,
		0x22,
		[](const DWORD event, ...)
		{
			run.peek_active = event == 0x21;
		},
		WINEVENT_OUTOFCONTEXT
	);

	MSG msg;
	BOOL ret;
	while ((ret = GetMessage(&msg, NULL, 0, 0)) != 0)
	{
		if (ret != -1)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			LastErrorHandle(Error::Level::Fatal, L"GetMessage failed!");
		}
	}

	// Close all open CPicker windows to avoid:
	// 1. Saving the colors currently previewed.
	// 2. Direct2D and Direct3D bothering us about leaks in debug mode (because CPicker gets unloaded after Direct2D so Direct2D
	//    does a leak check before CPicker even has a chance to cleanup anything). We don't care about them because we are closing,
	//    and if we have actual leaks, they won't be drowned in the noise since closing the window will make CPicker cleanup.
	win32::ClosePickers();

	// If it's a new instance, don't save or restore taskbar to default
	if (run.exit_reason != EXITREASON::NewInstance)
	{
		if (run.exit_reason != EXITREASON::UserActionNoSave)
		{
			Config::Save(run.config_file);
		}

		// Restore default taskbar appearance
		TogglePeek(true);
		worker.ReturnToStock();
	}

	return EXIT_SUCCESS;
}

#pragma endregion