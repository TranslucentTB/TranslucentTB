// Standard API
#include <filesystem>
#include <fstream>
#include <functional>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>

// Windows API
#include "arch.h"
#include <PathCch.h>
#include <sal.h>
#include <ShlObj.h>
#include <winrt/base.h>

// WIL
#include <wil/filesystem.h>

// RapidJSON
#include <rapidjson/document.h>
#include <rapidjson/encodings.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/prettywriter.h>

// Local stuff
#include "autostart.hpp"
#include "blacklist.hpp"
#include "config.hpp"
#include "constants.hpp"
#include "darkthememanager.hpp"
#include "resources/ids.h"
#include "smart/autofree.hpp"
#include "undoc/swca.hpp"
#include "taskbarattributeworker.hpp"
#include "taskdialogs/aboutdialog.hpp"
#include "taskdialogs/welcomedialog.hpp"
#include "tray/traycontextmenu.hpp"
#include "ttberror.hpp"
#include "ttblog.hpp"
#include "uwp.hpp"
#include "win32.hpp"
#include "windows/messagewindow.hpp"
#include "windows/window.hpp"
#include "windows/windowclass.hpp"

#pragma region Data

enum class EXITREASON {
	NewInstance,	 // New instance told us to exit
	UserAction,		 // Triggered by the user
	UserActionNoSave // Triggered by the user, but doesn't saves config
};

static struct {
	EXITREASON exit_reason = EXITREASON::UserAction;
	std::filesystem::path config_folder;
	std::filesystem::path config_file;
	std::filesystem::path exclude_file;
} run;

static const std::unordered_map<ACCENT_STATE, uint32_t> REGULAR_BUTTOM_MAP = {
	{ ACCENT_NORMAL,                     ID_REGULAR_NORMAL },
	{ ACCENT_ENABLE_TRANSPARENTGRADIENT, ID_REGULAR_CLEAR  },
	{ ACCENT_ENABLE_GRADIENT,            ID_REGULAR_OPAQUE },
	{ ACCENT_ENABLE_BLURBEHIND,          ID_REGULAR_BLUR   },
	{ ACCENT_ENABLE_ACRYLICBLURBEHIND,   ID_REGULAR_FLUENT }
};

static const std::unordered_map<ACCENT_STATE, uint32_t> MAXIMISED_BUTTON_MAP = {
	{ ACCENT_NORMAL,                     ID_MAXIMISED_NORMAL },
	{ ACCENT_ENABLE_TRANSPARENTGRADIENT, ID_MAXIMISED_CLEAR },
	{ ACCENT_ENABLE_GRADIENT,            ID_MAXIMISED_OPAQUE },
	{ ACCENT_ENABLE_BLURBEHIND,          ID_MAXIMISED_BLUR },
	{ ACCENT_ENABLE_ACRYLICBLURBEHIND,   ID_MAXIMISED_FLUENT }
};

static const std::unordered_map<ACCENT_STATE, uint32_t> START_BUTTON_MAP = {
	{ ACCENT_NORMAL,                     ID_START_NORMAL },
	{ ACCENT_ENABLE_TRANSPARENTGRADIENT, ID_START_CLEAR },
	{ ACCENT_ENABLE_GRADIENT,            ID_START_OPAQUE },
	{ ACCENT_ENABLE_BLURBEHIND,          ID_START_BLUR },
	{ ACCENT_ENABLE_ACRYLICBLURBEHIND,   ID_START_FLUENT }
};

static const std::unordered_map<ACCENT_STATE, uint32_t> CORTANA_BUTTON_MAP = {
	{ ACCENT_NORMAL,                     ID_CORTANA_NORMAL },
	{ ACCENT_ENABLE_TRANSPARENTGRADIENT, ID_CORTANA_CLEAR },
	{ ACCENT_ENABLE_GRADIENT,            ID_CORTANA_OPAQUE },
	{ ACCENT_ENABLE_BLURBEHIND,          ID_CORTANA_BLUR },
	{ ACCENT_ENABLE_ACRYLICBLURBEHIND,   ID_CORTANA_FLUENT }
};

static const std::unordered_map<ACCENT_STATE, uint32_t> TIMELINE_BUTTON_MAP = {
	{ ACCENT_NORMAL,                     ID_TIMELINE_NORMAL },
	{ ACCENT_ENABLE_TRANSPARENTGRADIENT, ID_TIMELINE_CLEAR },
	{ ACCENT_ENABLE_GRADIENT,            ID_TIMELINE_OPAQUE },
	{ ACCENT_ENABLE_BLURBEHIND,          ID_TIMELINE_BLUR },
	{ ACCENT_ENABLE_ACRYLICBLURBEHIND,   ID_TIMELINE_FLUENT }
};

static const std::unordered_map<PeekBehavior, uint32_t> PEEK_BUTTON_MAP = {
	{ PeekBehavior::AlwaysShow,                   ID_PEEK_SHOW },
	{ PeekBehavior::WindowMaximisedOnMainMonitor, ID_PEEK_DYNAMIC_MAIN_MONITOR },
	{ PeekBehavior::WindowMaximisedOnAnyMonitor,  ID_PEEK_DYNAMIC_ANY_MONITOR },
	{ PeekBehavior::DesktopIsForegroundWindow,    ID_PEEK_DYNAMIC_FOREGROUND_DESKTOP },
	{ PeekBehavior::AlwaysHide,                   ID_PEEK_HIDE }
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
	std::filesystem::path config_folder;
	try
	{
		config_folder = static_cast<std::wstring_view>(UWP::GetApplicationFolderPath(UWP::FolderType::Roaming));
	}
	WinrtExceptionCatch(Error::Level::Fatal, L"Getting application folder paths failed!")

	config_folder /= NAME;

	run.config_folder = std::move(config_folder);
	run.config_file = run.config_folder / CONFIG_FILE;
	run.exclude_file = run.config_folder / EXCLUDE_FILE;
}

void ApplyStock(std::wstring_view filename)
{
	std::filesystem::create_directory(run.config_folder);
	std::filesystem::copy_file(win32::GetExeLocation().parent_path() / filename, run.config_folder / filename);
}

bool CheckAndRunWelcome()
{
	if (!std::filesystem::is_directory(run.config_folder))
	{
		if (!WelcomeDialog(run.config_folder).Run())
		{
			return false;
		}
	}

	if (!std::filesystem::is_regular_file(run.config_file))
	{
		ApplyStock(CONFIG_FILE);
	}

	if (!std::filesystem::is_regular_file(run.exclude_file))
	{
		ApplyStock(EXCLUDE_FILE);
	}
	return true;
}

Config LoadConfig(const std::filesystem::path &file)
{
	using namespace rapidjson;

	Config cfg;

	std::wifstream fileStream(file);

	WIStreamWrapper jsonStream(fileStream);
	GenericDocument<UTF16LE<>> doc;
	doc.ParseStream<kParseCommentsFlag>(jsonStream);

	cfg.Deserialize(doc);

	return cfg;
}

void SaveConfig(const Config &cfg, const std::filesystem::path &file)
{
	using namespace rapidjson;

	std::wofstream fileStream(file);
	fileStream << L"// For reference on this format, see TODO" << std::endl;

	WOStreamWrapper jsonStream(fileStream);
	PrettyWriter<WOStreamWrapper, UTF16LE<>, UTF16LE<>> writer(jsonStream);

	writer.StartObject();
	cfg.Serialize(writer);
	writer.EndObject();
}

#pragma endregion

#pragma region Tray

void EnableAppearanceColor(TrayContextMenu::ContextMenuUpdater updater, unsigned int id, const OptionalTaskbarAppearance &appearance)
{
	updater.EnableItem(id, appearance.Enabled && appearance.Accent != ACCENT_NORMAL);
}

winrt::fire_and_forget RefreshMenu(const Config &cfg, TrayContextMenu::ContextMenuUpdater updater)
{
	// Fire off the task and do what we can do before blocking
	const auto task = Autostart::GetStartupState();
	updater.EnableItem(ID_AUTOSTART, false);
	updater.CheckItem(ID_AUTOSTART, false);
	updater.SetText(ID_AUTOSTART, L"Querying startup state...");


	const bool has_log = !Log::file().empty();
	updater.EnableItem(ID_OPENLOG, has_log);
	updater.SetText(ID_OPENLOG, has_log
		? L"Open log file"
		: Log::init_done()
			? L"Error when initializing log file"
			: L"Nothing has been logged yet"
	);

	updater.EnableItem(ID_SAVESETTINGS, !cfg.DisableSaving);

	updater.EnableItem(ID_REGULAR_COLOR, cfg.RegularAppearance.Accent != ACCENT_NORMAL);
	EnableAppearanceColor(updater, ID_MAXIMISED_COLOR, cfg.MaximisedWindowAppearance);
	EnableAppearanceColor(updater, ID_START_COLOR, cfg.StartOpenedAppearance);
	EnableAppearanceColor(updater, ID_CORTANA_COLOR, cfg.CortanaOpenedAppearance);
	EnableAppearanceColor(updater, ID_TIMELINE_COLOR, cfg.TimelineOpenedAppearance);

	// Block until it finishes
	const auto state = co_await task;
	updater.EnableItem(ID_AUTOSTART,
		!(state == Autostart::StartupState::DisabledByUser || state == Autostart::StartupState::DisabledByPolicy || state == Autostart::StartupState::EnabledByPolicy));
	updater.CheckItem(ID_AUTOSTART, state == Autostart::StartupState::Enabled || state == Autostart::StartupState::EnabledByPolicy);

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
	updater.SetText(ID_AUTOSTART, std::move(autostart_text));
}

long ExitApp(EXITREASON reason, ...)
{
	run.exit_reason = reason;
	PostQuitMessage(0);
	return 0;
}

#pragma endregion

#pragma region Startup

bool IsSingleInstance()
{
	static wil::unique_mutex mutex;

	if (!mutex)
	{
		const bool opened = mutex.try_open(MUTEX_GUID);
		if (!opened)
		{
			mutex.create(MUTEX_GUID);
		}

		return !opened;
	}
	else
	{
		return true;
	}
}

void BindAppearance(TrayContextMenu &tray, TaskbarAppearance &appearance, unsigned int colorId, const std::unordered_map<ACCENT_STATE, uint32_t> &map)
{
	tray.BindColor(colorId, appearance.Color);
	tray.BindEnum(appearance.Accent, map);
}

void BindAppearance(TrayContextMenu &tray, OptionalTaskbarAppearance &appearance, unsigned int enableId, unsigned int colorId, const std::unordered_map<ACCENT_STATE, uint32_t> &map)
{
	tray.BindBool(enableId, appearance.Enabled, TrayContextMenu::Toggle);
	BindAppearance(tray, appearance, colorId, map);
	for (const auto &[_, id] : map)
	{
		tray.BindBool(id, appearance.Enabled, TrayContextMenu::ControlsEnabled);
	}
}

void InitializeTray(HINSTANCE hInstance, Config &cfg)
{
	static MessageWindow window(TRAY_WINDOW, NAME, hInstance);
	DarkThemeManager::EnableDarkModeForWindow(window);

	static TaskbarAttributeWorker worker(hInstance, cfg);
	static auto watcher = wil::make_folder_watcher(run.config_folder.c_str(), false, wil::FolderChangeEvents::LastWriteTime, [&cfg]
	{
		cfg = LoadConfig(run.config_file);
	});

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

	window.RegisterCallback(WM_ENDSESSION, [&cfg](WPARAM wParam, ...)
	{
		if (wParam)
		{
			// The app can be closed anytime after processing this message. Save the settings.
			SaveConfig(cfg, run.config_file);
		}

		return 0;
	});


	if (!cfg.HideTray)
	{
		static TrayContextMenu tray(window, MAKEINTRESOURCE(IDI_TRAYWHITEICON), MAKEINTRESOURCE(IDR_TRAY_MENU), hInstance);
		DarkThemeManager::EnableDarkModeForTrayIcon(tray, MAKEINTRESOURCE(IDI_TRAYWHITEICON), MAKEINTRESOURCE(IDI_TRAYBLACKICON));


		BindAppearance(tray, cfg.RegularAppearance, ID_REGULAR_COLOR, REGULAR_BUTTOM_MAP);
		BindAppearance(tray, cfg.MaximisedWindowAppearance, ID_MAXIMISED, ID_MAXIMISED_COLOR, MAXIMISED_BUTTON_MAP);
		BindAppearance(tray, cfg.StartOpenedAppearance, ID_START, ID_START_COLOR, START_BUTTON_MAP);
		BindAppearance(tray, cfg.CortanaOpenedAppearance, ID_CORTANA, ID_CORTANA_COLOR, CORTANA_BUTTON_MAP);
		BindAppearance(tray, cfg.TimelineOpenedAppearance, ID_TIMELINE, ID_TIMELINE_COLOR, TIMELINE_BUTTON_MAP);


		tray.BindEnum(cfg.Peek, PEEK_BUTTON_MAP);
		tray.BindBool(ID_REGULAR_ON_PEEK, cfg.UseRegularAppearanceWhenPeeking, TrayContextMenu::Toggle);


		tray.RegisterContextMenuCallback(ID_OPENLOG, []
		{
			Log::Flush();
			win32::EditFile(Log::file());
		});
		tray.BindBool(ID_VERBOSE, cfg.VerboseLog, TrayContextMenu::Toggle);
		tray.RegisterContextMenuCallback(ID_SAVESETTINGS, [&cfg]
		{
			SaveConfig(cfg, run.config_file);
			std::thread(std::bind(&MessageBox, Window::NullWindow, L"Settings have been saved.", NAME, MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND)).detach();
		});
		tray.RegisterContextMenuCallback(ID_RELOADSETTINGS, [&cfg]
		{
			cfg = LoadConfig(run.config_file);
		});
		tray.RegisterContextMenuCallback(ID_EDITSETTINGS, [&cfg]
		{
			SaveConfig(cfg, run.config_file);
			win32::EditFile(run.config_file);
		});
		tray.RegisterContextMenuCallback(ID_RETURNTODEFAULTSETTINGS, [&cfg]
		{
			cfg = { };
			SaveConfig(cfg, run.config_file);
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


		tray.RegisterContextMenuCallback(ID_AUTOSTART, []() -> winrt::fire_and_forget
		{
			co_await Autostart::SetStartupState(
				co_await Autostart::GetStartupState() == Autostart::StartupState::Enabled
					? Autostart::StartupState::Disabled
					: Autostart::StartupState::Enabled
			);
		});
		tray.RegisterContextMenuCallback(ID_TIPS, std::bind(&win32::OpenLink,
			L"https://github.com/TranslucentTB/TranslucentTB/wiki/Tips-and-tricks-for-a-better-looking-taskbar"));
		tray.RegisterContextMenuCallback(ID_EXIT, std::bind(&ExitApp, EXITREASON::UserAction));


		tray.RegisterCustomRefresh(std::bind(&RefreshMenu, std::ref(cfg), std::placeholders::_1));
	}
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ wchar_t *, _In_ int)
{
	win32::HardenProcess();
	try
	{
		winrt::init_apartment(winrt::apartment_type::single_threaded);
	}
	WinrtExceptionCatch(Error::Level::Fatal, L"Initialization of Windows Runtime failed.")

	// If there already is another instance running, tell it to exit
	if (!IsSingleInstance())
	{
		Window::Find(TRAY_WINDOW, NAME).send_message(WM_NEWTTBINSTANCE);
	}

	DarkThemeManager::AllowDarkModeForApp();

	// TODO: std::filesystem::filesystem_exception handling

	// Get configuration file paths
	GetPaths();

	// If the configuration files don't exist, restore the files and show welcome to the users
	if (!CheckAndRunWelcome())
	{
		return EXIT_FAILURE;
	}

	// Parse our configuration
	static Config cfg = LoadConfig(run.config_file);
	//TODO if (!Config::ParseCommandLine())
	//{
	//	return EXIT_SUCCESS;
	//}

	// Initialize GUI
	InitializeTray(hInstance, cfg);

	// Run the main program loop. When this method exits, TranslucentTB itself is about to exit.
	MessageWindow::RunMessageLoop();

	// If it's a new instance, don't save or restore taskbar to default
	if (run.exit_reason != EXITREASON::NewInstance)
	{
		if (run.exit_reason != EXITREASON::UserActionNoSave)
		{
			SaveConfig(cfg, run.config_file);
		}
	}

	winrt::uninit_apartment();

	return EXIT_SUCCESS;
}

#pragma endregion