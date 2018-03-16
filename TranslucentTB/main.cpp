// Standard API
#include <chrono>
#include <ctime>
#include <cwchar>
#include <cwctype>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

// Windows API
#include <atlbase.h>
#include <comdef.h>
#include <dwmapi.h>
#include <PathCch.h>
#include <Psapi.h>
#include <shellapi.h>
#include <ShellScalingApi.h>
#include <ShlObj.h>
#include <wrl/wrappers/corewrappers.h>

// https://msdn.microsoft.com/en-us/library/windows/desktop/aa365247(v=vs.85).aspx
// "Note: The maximum path of 32,767 characters is approximate."
// smh
#define LONG_PATH 33000

// UWP
#ifdef STORE
#include "UWP.hpp"
#endif

// For the context menu
#include "resource.h"

#include "swcadata.hpp"
#include "config.hpp"
#include "taskbar.hpp"
#include "tray.hpp"
#include "util.hpp"
#include "win32.hpp"
#include "app.hpp"
#include "ttberror.hpp"
#include "ttblog.hpp"
#include "autostart.hpp"
#include "windowhelper.hpp"
#include "AutoFree.hpp"


#pragma region Structures

static struct OPTIONS
{
	swca::ACCENT taskbar_appearance = swca::ACCENT_ENABLE_BLURBEHIND;
	uint32_t color = 0x00000000;
	bool dynamic_ws = false;
	swca::ACCENT dynamic_ws_taskbar_appearance = swca::ACCENT_ENABLE_BLURBEHIND;
	uint32_t dynamic_ws_color = 0x00000000;
	bool dynamic_ws_normal_on_peek = true;
	bool dynamic_start = false;
	Taskbar::AEROPEEK peek = Taskbar::AEROPEEK::Enabled;
	std::vector<std::wstring> blacklisted_classes;
	std::vector<std::wstring> blacklisted_filenames;
	std::vector<std::wstring> blacklisted_titles;
	uint16_t sleep_time = 10;
} opt;

static struct RUNTIMESTATE
{
	Tray::EXITREASON exit_reason = Tray::UserAction;
	HWND main_taskbar;
	std::unordered_map<HMONITOR, Taskbar::TASKBARPROPERTIES> taskbars;
	bool should_show_peek;
	bool is_running = true;
	HMENU tray_popup;
	NOTIFYICONDATA tray;
	bool fluent_available = false;
	std::wstring config_folder;
	std::wstring config_file;
	std::wstring exclude_file;
	int cache_hits;
	bool peek_active = false;
	HWINEVENTHOOK peek_hook;
} run;

#pragma endregion

#pragma region That one function that does all the magic

void SetWindowBlur(const HWND &hWnd, const swca::ACCENT &appearance, const uint32_t &color)
{
	if (user32::SetWindowCompositionAttribute)
	{
		swca::ACCENTPOLICY policy = {
			appearance,
			2,
			(color & 0xFF00FF00) + ((color & 0x00FF0000) >> 16) + ((color & 0x000000FF) << 16),
			0
		};

		if (policy.nAccentState == swca::ACCENT_NORMAL)
		{
			// Handle fake value
			policy.nAccentState = run.fluent_available ? swca::ACCENT_ENABLE_FLUENT : swca::ACCENT_ENABLE_TRANSPARENTGRADIENT;
			policy.nColor = 0x99000000;
		}
		else if (policy.nAccentState == swca::ACCENT_ENABLE_FLUENT && policy.nColor >> 24 == 0x00)
		{
			// Fluent mode doesn't likes a completely 0 opacity
			policy.nColor = (0x01 << 24) + (policy.nColor & 0x00FFFFFF);
		}

		swca::WINCOMPATTRDATA data = {
			swca::WCA_ACCENT_POLICY,
			&policy,
			sizeof(policy)
		};

		user32::SetWindowCompositionAttribute(hWnd, &data);
	}
}

#pragma endregion

#pragma region Configuration

void GetPaths()
{
#ifndef STORE
	AutoFree<wchar_t> appData(nullptr, CoTaskMemFree);
	Error::Handle(SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_DEFAULT, NULL, &appData), Error::Level::Fatal, L"Failed to determine configuration files locations!");

	std::vector<wchar_t> temp(LONG_PATH);
	if (!GetTempPath(LONG_PATH, temp.data()))
	{
		Error::Handle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Fatal, L"Failed to determine log files locations!");
	}

	AutoFree<wchar_t> log_folder;
	Error::Handle(PathAllocCombine(temp.data(), App::NAME.c_str(), PATHCCH_ALLOW_LONG_PATHS, &log_folder), Error::Level::Fatal, L"Failed to combine temporary folder and application name!");

	Log::Folder = log_folder;
#else
	try
	{
		std::wstring appData_str = UWP::GetApplicationFolderPath(UWP::FolderType::Roaming);
		const wchar_t *appData = appData_str.c_str();

		Log::Folder = UWP::GetApplicationFolderPath(UWP::FolderType::Temporary);

#endif

	AutoFree<wchar_t> configFolder;
	AutoFree<wchar_t> configFile;
	AutoFree<wchar_t> excludeFile;

	Error::Handle(PathAllocCombine(appData, App::NAME.c_str(), PATHCCH_ALLOW_LONG_PATHS, &configFolder), Error::Level::Fatal, L"Failed to combine AppData folder and application name!");
	Error::Handle(PathAllocCombine(configFolder, Config::CONFIG_FILE.c_str(), PATHCCH_ALLOW_LONG_PATHS, &configFile), Error::Level::Fatal, L"Failed to combine config folder and config file!");
	Error::Handle(PathAllocCombine(configFolder, Config::EXCLUDE_FILE.c_str(), PATHCCH_ALLOW_LONG_PATHS, &excludeFile), Error::Level::Fatal, L"Failed to combine config folder and exclude file!");

	run.config_folder = configFolder;
	run.config_file = configFile;
	run.exclude_file = excludeFile;

#ifdef STORE
	}
	catch (const winrt::hresult_error &error)
	{
		Error::Handle(error.code(), Error::Level::Fatal, L"Getting application folder paths failed!");
	}
#endif
}

void ApplyStock(const std::wstring &filename)
{
	DWORD exeFolder_size = LONG_PATH;
	std::vector<wchar_t> exeFolder(exeFolder_size);
	if (!QueryFullProcessImageName(GetCurrentProcess(), 0, exeFolder.data(), &exeFolder_size))
	{
		Error::Handle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Error, L"Failed to determine executable location!");
		return;
	}
	std::wstring exeFolder_str(exeFolder.data());
	exeFolder_str = exeFolder_str.substr(0, exeFolder_str.find_last_of(L"/\\") + 1);

	AutoFree<wchar_t> stockFile;
	if (!Error::Handle(PathAllocCombine(exeFolder_str.c_str(), filename.c_str(), PATHCCH_ALLOW_LONG_PATHS, &stockFile), Error::Level::Error, L"Failed to combine executable folder and config file!"))
	{
		return;
	}

	AutoFree<wchar_t> configFile;
	if (!Error::Handle(PathAllocCombine(run.config_folder.c_str(), filename.c_str(), PATHCCH_ALLOW_LONG_PATHS, &configFile), Error::Level::Error, L"Failed to combine config folder and config file!"))
	{
		return;
	}

	if (!win32::IsDirectory(run.config_folder))
	{
		if (!CreateDirectory(run.config_folder.c_str(), NULL))
		{
			if (!Error::Handle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Error, L"Creating configuration files directory failed!"))
			{
				return;
			}
		}
	}

	if (!CopyFile(stockFile, configFile, FALSE))
	{
		Error::Handle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Error, L"Copying stock configuration file failed!");
	}
}

bool CheckAndRunWelcome()
{
	if (!win32::IsDirectory(run.config_folder))
	{
		// String concatenation is hard OK
		std::wstring message;
		message += L"Welcome to ";
		message += App::NAME;
		message += L"!\n\n";
		message += L"You can tweak the taskbar's appearance with the tray icon. If it's your cup of tea, you can also edit the configuration files, located at \"";
		message += run.config_folder;
		message += '"';
		message += L"\n\nDo you agree to the GPLv3 license?";

		if (MessageBox(NULL, message.c_str(), App::NAME.c_str(), MB_ICONINFORMATION | MB_YESNO | MB_SETFOREGROUND) != IDYES)
		{
			return false;
		}
	}
	if (!win32::FileExists(run.config_file))
	{
		ApplyStock(Config::CONFIG_FILE);
	}
	if (!win32::FileExists(run.exclude_file))
	{
		ApplyStock(Config::EXCLUDE_FILE);
	}
	return true;
}

inline void UnknownValue(const std::wstring &key, const std::wstring &value)
{
	Log::OutputMessage(L"Unknown value found in configuration file: " + value + L" (for key: " + key + L")");
}

void ParseSingleConfigOption(const std::wstring &arg, const std::wstring &value)
{
	if (arg == L"accent")
	{
		if (value == L"blur")
		{
			opt.taskbar_appearance = swca::ACCENT_ENABLE_BLURBEHIND;
		}
		else if (value == L"opaque")
		{
			opt.taskbar_appearance = swca::ACCENT_ENABLE_GRADIENT;
		}
		else if (value == L"transparent" || value == L"translucent" || value == L"clear")
		{
			opt.taskbar_appearance = swca::ACCENT_ENABLE_TRANSPARENTGRADIENT;
		}
		else if (value == L"normal")
		{
			opt.taskbar_appearance = swca::ACCENT_NORMAL;
		}
		else if (value == L"fluent" && run.fluent_available)
		{
			opt.taskbar_appearance = swca::ACCENT_ENABLE_FLUENT;
		}
		else
		{
			UnknownValue(arg, value);
		}
	}
	else if (arg == L"sleep-time")
	{
		try
		{
			int sleep_time = std::stoi(value);
			opt.sleep_time = sleep_time;
		}
		catch (std::invalid_argument)
		{
			Log::OutputMessage(L"Could not parse sleep time found in configuration file: " + value);
		}
	}
	else if (arg == L"dynamic-ws")
	{
		if (value == L"true" || value == L"enable")
		{
			opt.dynamic_ws = true;
		}
		else if (value == L"false" || value == L"disable")
		{
			opt.dynamic_ws = false;
		}
		else
		{
			UnknownValue(arg, value);
		}
	}
	else if (arg == L"dynamic-ws-accent")
	{
		if (value == L"blur")
		{
			opt.dynamic_ws_taskbar_appearance = swca::ACCENT_ENABLE_BLURBEHIND;
		}
		else if (value == L"opaque")
		{
			opt.dynamic_ws_taskbar_appearance = swca::ACCENT_ENABLE_GRADIENT;
		}
		else if (value == L"clear")
		{
			opt.dynamic_ws_taskbar_appearance = swca::ACCENT_ENABLE_TRANSPARENTGRADIENT;
		}
		else if (value == L"normal")
		{
			opt.dynamic_ws_taskbar_appearance = swca::ACCENT_NORMAL;
		}
		else if (value == L"fluent" && run.fluent_available)
		{
			opt.dynamic_ws_taskbar_appearance = swca::ACCENT_ENABLE_FLUENT;
		}
		else
		{
			UnknownValue(arg, value);
		}
	}
	else if (arg == L"dynamic-start")
	{
		if (value == L"true" || value == L"enable")
		{
			opt.dynamic_start = true;
		}
		else if (value == L"false" || value == L"disable")
		{
			opt.dynamic_start = false;
		}
		else
		{
			UnknownValue(arg, value);
		}
	}
	else if (arg == L"color" || arg == L"tint")
	{
		std::wstring color_value = Util::Trim(value);

		if (color_value.find_first_of('#') == 0)
		{
			color_value = color_value.substr(1, color_value.length() - 1);
		}
		else if (color_value.find(L"0x") == 0)
		{
			color_value = color_value.substr(2, color_value.length() - 2);
		}

		// Get only the last 6 characters, keeps compatibility with old version.
		// It stored AARRGGBB in color, but now we store it as RRGGBB.
		// We read AA from opacity instead, which the old version also saved alpha to.
		if (color_value.length() > 6)
		{
			color_value = color_value.substr(color_value.length() - 6, 6);
		}

		try
		{
			opt.color = (opt.color & 0xFF000000) + (std::stoi(color_value, nullptr, 16) & 0x00FFFFFF);
		}
		catch (std::invalid_argument)
		{
			Log::OutputMessage(L"Could not parse color found in configuration file: " + value + L" (after processing: " + color_value + L")");
		}
	}
	else if (arg == L"dynamic-ws-color")
	{
		std::wstring color_value = Util::Trim(value);

		if (color_value.find_first_of('#') == 0)
		{
			color_value = color_value.substr(1, color_value.length() - 1);
		}
		else if (color_value.find(L"0x") == 0)
		{
			color_value = color_value.substr(2, color_value.length() - 2);
		}

		// Get only the last 6 characters, keeps compatibility with old version.
		// It stored AARRGGBB in color, but now we store it as RRGGBB.
		// We read AA from opacity instead, which the old version also saved alpha to.
		if (color_value.length() > 6)
		{
			color_value = color_value.substr(color_value.length() - 6, 6);
		}

		try
		{
			opt.dynamic_ws_color = (opt.dynamic_ws_color & 0xFF000000) + (std::stoi(color_value, nullptr, 16) & 0x00FFFFFF);
		}
		catch (std::invalid_argument)
		{
			Log::OutputMessage(L"Could not parse dynamic windows color found in configuration file: " + value + L" (after processing: " + color_value + L")");
		}
	}
	else if (arg == L"opacity")
	{
		int parsed;
		try
		{
			parsed = std::stoi(value);
		}
		catch (std::invalid_argument)
		{
			Log::OutputMessage(L"Could not parse opacity found in configuration file: " + value);
			return;
		}

		if (parsed < 0)
		{
			parsed = 0;
		}
		else if (parsed > 255)
		{
			parsed = 255;
		}

		opt.color = (parsed << 24) + (opt.color & 0x00FFFFFF);
	}
	else if (arg == L"dynamic-ws-opacity")
	{
		int parsed;
		try
		{
			parsed = std::stoi(value);
		}
		catch (std::invalid_argument)
		{
			Log::OutputMessage(L"Could not parse dynamic windows opacity found in configuration file: " + value);
			return;
		}

		if (parsed < 0)
		{
			parsed = 0;
		}
		else if (parsed > 255)
		{
			parsed = 255;
		}

		opt.dynamic_ws_color = (parsed << 24) + (opt.dynamic_ws_color & 0x00FFFFFF);
	}
	else if (arg == L"peek")
	{
		if (value == L"hide")
		{
			opt.peek = Taskbar::AEROPEEK::Disabled;
		}
		else if (value == L"dynamic")
		{
			opt.peek = Taskbar::AEROPEEK::Dynamic;
		}
		else if (value == L"show")
		{
			opt.peek = Taskbar::AEROPEEK::Enabled;
		}
		else
		{
			UnknownValue(arg, value);
		}
	}
	else if (arg == L"dynamic-ws-normal-on-peek")
	{
		if (value == L"true" || value == L"enable")
		{
			opt.dynamic_ws_normal_on_peek = true;
		}
		else if (value == L"false" || value == L"disable")
		{
			opt.dynamic_ws_normal_on_peek = false;
		}
		else
		{
			UnknownValue(arg, value);
		}
	}
	else if (arg == L"verbose")
	{
		if (value == L"true" || value == L"enable")
		{
			Config::VERBOSE = true;
		}
		else if (value == L"false" || value == L"disable")
		{
			Config::VERBOSE = false;
		}
		else
		{
			UnknownValue(arg, value);
		}
	}
	else if (arg == L"max-cache-hits")
	{
		try
		{
			int cache_hits = std::stoi(value);
			Config::CACHE_HIT_MAX = cache_hits;
		}
		catch (std::invalid_argument)
		{
			Log::OutputMessage(L"Could not parse max cache hits found in configuration file: " + value);
		}
	}
	else
	{
		Log::OutputMessage(L"Unknown key found in configuration file: " + arg);
	}
}

void ParseConfigFile()
{
	std::wifstream configstream(run.config_file);

	for (std::wstring line; std::getline(configstream, line); )
	{
		if (line.empty())
		{
			continue;
		}

		// Skip comments
		size_t comment_index = line.find(L';');
		if (comment_index == 0)
		{
			continue;
		}
		else if (comment_index != std::wstring::npos)
		{
			line = line.substr(0, comment_index);
		}

		size_t split_index = line.find(L'=');
		if (split_index != std::wstring::npos)
		{
			std::wstring key = line.substr(0, split_index);
			std::wstring val = line.substr(split_index + 1, line.length() - split_index - 1);
			ParseSingleConfigOption(key, val);
		}
		else
		{
			Log::OutputMessage(L"Invalid line in configuration file");
		}
	}
}

void SaveConfigFile()
{
	std::wstring configfile(run.config_file);
	if (!configfile.empty())
	{
		using namespace std;
		wofstream configstream(configfile);

		configstream << L"; Taskbar appearance: fluent, opaque, clear, normal, or blur (default)." << endl;

		configstream << L"accent=";
		switch (opt.taskbar_appearance)
		{
		case swca::ACCENT_ENABLE_GRADIENT:
			configstream << L"opaque";
			break;
		case swca::ACCENT_ENABLE_TRANSPARENTGRADIENT:
			configstream << L"clear";
			break;
		case swca::ACCENT_ENABLE_BLURBEHIND:
			configstream << L"blur";
			break;
		case swca::ACCENT_NORMAL:
			configstream << L"normal";
			break;
		case swca::ACCENT_ENABLE_FLUENT:
			configstream << L"fluent";
			break;
		}
		configstream << endl;
		configstream << L"; Color and opacity of the taskbar." << endl;

		configstream << L"color=";
		configstream << right << setw(6) << setfill<wchar_t>('0') << hex << (opt.color & 0x00FFFFFF);
		configstream << L" ; A color in hexadecimal notation." << endl;

		configstream << L"opacity=";
		configstream << left << setw(3) << setfill<wchar_t>(' ') << to_wstring((opt.color & 0xFF000000) >> 24);
		configstream << L"  ; A value in the range 0 to 255." << endl;

		configstream << endl;
		configstream << L"; Dynamic Windows and Start Menu" << endl;
		configstream << L"; Available states are: clear, normal, fluent, opaque, normal, or blur (default)." << endl;
		configstream << L"; dynamic windows has its own color and opacity configs." << endl;
		configstream << L"; by enabling dynamic-ws-normal-on-peek, dynamic windows will behave as if no window is maximised when using Aero Peek." << endl;
		configstream << L"; you can also set the accent, color and opacity values, which will represent the state of dynamic windows when there is no window maximised." << endl;
		configstream << L"; dynamic start returns the taskbar to normal appearance when the start menu is opened." << endl;

		configstream << L"dynamic-ws=" << (opt.dynamic_ws ? L"enable" : L"disable") << endl;

		configstream << L"dynamic-ws-accent=";
		switch (opt.dynamic_ws_taskbar_appearance)
		{
		case swca::ACCENT_ENABLE_BLURBEHIND:
			configstream << L"blur";
			break;
		case swca::ACCENT_ENABLE_GRADIENT:
			configstream << L"opaque";
			break;
		case swca::ACCENT_ENABLE_TRANSPARENTGRADIENT:
			configstream << L"clear";
			break;
		case swca::ACCENT_NORMAL:
			configstream << L"normal";
			break;
		case swca::ACCENT_ENABLE_FLUENT:
			configstream << L"fluent";
			break;
		}
		configstream << endl;


		configstream << L"dynamic-ws-color=";
		configstream << right << setw(6) << setfill<wchar_t>('0') << hex << (opt.dynamic_ws_color & 0x00FFFFFF);
		configstream << L" ; A color in hexadecimal notation." << endl;

		configstream << L"dynamic-ws-opacity=";
		configstream << left << setw(3) << setfill<wchar_t>(' ') << to_wstring((opt.dynamic_ws_color & 0xFF000000) >> 24);
		configstream << L"  ; A value in the range 0 to 255." << endl;

		configstream << L"dynamic-ws-normal-on-peek=" << (opt.dynamic_ws_normal_on_peek ? L"enable" : L"disable") << endl;
		configstream << L"dynamic-start=" << (opt.dynamic_start ? L"enable" : L"disable") << endl;

		configstream << endl;
		configstream << L"; Controls how the Aero Peek button behaves (dynamic, show or hide)" << endl;
		configstream << L"peek=";
		switch (opt.peek)
		{
		case Taskbar::AEROPEEK::Disabled:
			configstream << L"hide";
			break;
		case Taskbar::AEROPEEK::Dynamic:
			configstream << L"dynamic";
			break;
		case Taskbar::AEROPEEK::Enabled:
			configstream << L"show";
			break;
		}
		configstream << endl;

		configstream << endl;
		configstream << L"; Advanced settings" << endl;
		configstream << L"; more informative logging. can make huge log files." << endl;
		configstream << L"verbose=" << (Config::VERBOSE ? L"enable" : L"disable") << endl;
		configstream << L"; sleep time in milliseconds, a shorter time reduces flicker when opening start, but results in higher CPU usage." << endl;
		configstream << L"sleep-time=" << to_wstring(opt.sleep_time) << endl;
		configstream << L"; maximum number of times the blacklist cache can be hit before getting cleared." << endl;
		configstream << L"max-cache-hits=" << to_wstring(Config::CACHE_HIT_MAX) << endl;
	}
}

void ParseBlacklistFile()
{
	std::wstring filename(run.exclude_file);
	for (std::vector<std::wstring> vector : { opt.blacklisted_classes, opt.blacklisted_filenames, opt.blacklisted_titles })
	{
		vector.clear(); // Clear our vectors
	}

	std::wifstream excludesfilestream(filename);

	const std::wstring delimiter = L","; // Change to change the char(s) used to split,

	for (std::wstring line; std::getline(excludesfilestream, line); )
	{
		if (line.empty())
		{
			continue;
		}

		size_t comment_index = line.find(L';');
		if (comment_index == 0)
		{
			continue;
		}
		else if (comment_index != std::wstring::npos)
		{
			line = line.substr(0, comment_index);
		}

		if (line.length() > delimiter.length())
		{
			if (line.compare(line.length() - delimiter.length(), delimiter.length(), delimiter))
			{
				line.append(delimiter);
			}
		}

		std::wstring line_lowercase = line;
		Util::ToLower(line_lowercase);

		if (line_lowercase.substr(0, 5) == L"class")
		{
			Util::AddValuesToVectorByDelimiter(delimiter, opt.blacklisted_classes, line);
		}
		else if (line_lowercase.substr(0, 5) == L"title" || line.substr(0, 13) == L"windowtitle")
		{
			Util::AddValuesToVectorByDelimiter(delimiter, opt.blacklisted_titles, line);
		}
		else if (line_lowercase.substr(0, 7) == L"exename")
		{
			Util::AddValuesToVectorByDelimiter(delimiter, opt.blacklisted_filenames, line_lowercase);
		}
		else
		{
			Log::OutputMessage(L"Invalid line in dynamic window blacklist file");
		}
	}
}

#pragma endregion

#pragma region Utilities

void StartLogger()
{
	if (!win32::IsDirectory(Log::Folder.c_str()))
	{
		if (!CreateDirectory(Log::Folder.c_str(), NULL))
		{
			Error::Handle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Error, L"Creating log files directory failed!");
		}
	}

	std::time_t unix_epoch = std::time(0);
	std::wstring log_filename = std::to_wstring(unix_epoch) + L".log";

	AutoFree<wchar_t> log_file;
	if (!Error::Handle(PathAllocCombine(Log::Folder.c_str(), log_filename.c_str(), PATHCCH_ALLOW_LONG_PATHS, &log_file), Error::Level::Error, L"Failed to combine log folder and log filename! Log file not available during this session."))
	{
		return;
	}

	Log::Instance = new Logger(static_cast<wchar_t *>(log_file));
	Log::File = log_file;

	Log::OutputMessage(L"Basic initialization completed.");
}

void RefreshHandles()
{
	if (Config::VERBOSE)
	{
		Log::OutputMessage(L"Refreshing taskbar handles");
	}
	HWND secondtaskbar = NULL;
	Taskbar::TASKBARPROPERTIES _properties;

	// Older handles are invalid, so clear the map to be ready for new ones
	run.taskbars.clear();

	_properties.hwnd = run.main_taskbar = FindWindow(L"Shell_TrayWnd", NULL);
	_properties.state = Taskbar::Normal;
	run.taskbars.insert(std::make_pair(MonitorFromWindow(run.main_taskbar, MONITOR_DEFAULTTOPRIMARY), _properties));

	while ((secondtaskbar = FindWindowEx(0, secondtaskbar, L"Shell_SecondaryTrayWnd", NULL)) != 0)
	{
		_properties.hwnd = secondtaskbar;
		_properties.state = Taskbar::Normal;
		run.taskbars.insert(std::make_pair(MonitorFromWindow(secondtaskbar, MONITOR_DEFAULTTOPRIMARY), _properties));
	}
}

void TogglePeek(const bool &status)
{
	static bool cached_peek = true;
	static HWND cached_taskbar = run.main_taskbar;

	if (status != cached_peek || cached_taskbar != run.main_taskbar)
	{
		HWND _tray = FindWindowEx(run.main_taskbar, NULL, L"TrayNotifyWnd", NULL);
		HWND _peek = FindWindowEx(_tray, NULL, L"TrayShowDesktopButtonWClass", NULL);
		HWND _overflow = FindWindowEx(_tray, NULL, L"Button", NULL);

		ShowWindow(_peek, status ? SW_SHOWNORMAL : SW_HIDE);

		// This is a really terrible hack, but it's the only way I found to make the changes reflect instantly.
		// Toggles the overflow area popup twice. Nearly imperceptible.
		// If you have a better solution, let me know or send a pull request
		SendMessage(_overflow, WM_LBUTTONUP, NULL, NULL);
		SendMessage(_overflow, WM_LBUTTONUP, NULL, NULL);

		cached_peek = status;
		cached_taskbar = run.main_taskbar;
	}
}

void ClearBlacklistCache()
{
	run.cache_hits = Config::CACHE_HIT_MAX + 1;
}

bool OutputBlacklistMatchToLog(const HWND &hwnd, const bool &match)
{
	if (Config::VERBOSE)
	{
		std::wstring className = WindowHelper::GetWindowClass(hwnd);
		std::wstring title = WindowHelper::GetWindowTitle(hwnd);
		std::wstring exeName = WindowHelper::GetWindowFile(hwnd);

		std::wstringstream message;
		message << (match ? L"Blacklist match found for window: " : L"No blacklist match found for window: ");
		message << hwnd << L" [" << className << L"] [" << exeName << L"] [" << title << L"]";

		Log::OutputMessage(message.str());
	}

	return match;
}

bool IsWindowBlacklisted(const HWND &hWnd)
{
	static std::unordered_map<HWND, bool> blacklist_cache;

	if (run.cache_hits <= Config::CACHE_HIT_MAX && blacklist_cache.count(hWnd) > 0)
	{
		run.cache_hits++;
		return blacklist_cache[hWnd];
	}
	else
	{
		if (run.cache_hits > Config::CACHE_HIT_MAX)
		{
			if (Config::VERBOSE)
			{
				Log::OutputMessage(L"Maximum number of " + std::to_wstring(Config::CACHE_HIT_MAX) + L" cache hits reached, clearing blacklist cache.");
			}
			run.cache_hits = 0;
			blacklist_cache.clear();
		}

		// This is the fastest because we do the less string manipulation, so always try it first
		if (opt.blacklisted_classes.size() > 0)
		{
			std::wstring className = WindowHelper::GetWindowClass(hWnd);
			for (const std::wstring &value : opt.blacklisted_classes)
			{
				if (className == value)
				{
					return OutputBlacklistMatchToLog(hWnd, blacklist_cache[hWnd] = true);
				}
			}
		}

		// Try it second because idk
		// Window names can change, but I don't think it will be a big issue if we cache it.
		// If it ends up affecting stuff, we can remove it from caching easily.
		if (opt.blacklisted_titles.size() > 0)
		{
			std::wstring windowTitle = WindowHelper::GetWindowTitle(hWnd);
			for (const std::wstring &value : opt.blacklisted_titles)
			{
				if (windowTitle.find(value) != std::wstring::npos)
				{
					return OutputBlacklistMatchToLog(hWnd, blacklist_cache[hWnd] = true);
				}
			}
		}

		// GetModuleFileNameEx is quite expensive according to the tracing tools, so use it as last resort.
		if (opt.blacklisted_filenames.size() > 0)
		{
			std::wstring exeName = WindowHelper::GetWindowFile(hWnd);
			Util::ToLower(exeName);
			for (const std::wstring &value : opt.blacklisted_filenames)
			{
				if (exeName == value)
				{
					return OutputBlacklistMatchToLog(hWnd, blacklist_cache[hWnd] = true);
				}
			}
		}

		return OutputBlacklistMatchToLog(hWnd, blacklist_cache[hWnd] = false);
	}
}

#pragma endregion

#pragma region Tray

DWORD CheckPopupItem(const uint32_t &item_to_check, const bool &state)
{
	return CheckMenuItem(run.tray_popup, item_to_check, MF_BYCOMMAND | (state ? MF_CHECKED : MF_UNCHECKED) | MF_ENABLED);
}

bool EnablePopupItem(const uint32_t &item_to_enable, const bool &state)
{
	return EnableMenuItem(run.tray_popup, item_to_enable, MF_BYCOMMAND | (state ? MF_ENABLED : MF_GRAYED));
}

bool CheckPopupRadioItem(const uint32_t &from, const uint32_t &to, const uint32_t &item_to_check)
{
	return CheckMenuRadioItem(run.tray_popup, from, to, item_to_check, MF_BYCOMMAND);
}

void ChangePopupItemText(const uint32_t &item, const std::wstring &new_text)
{
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wmissing-field-initializers"
	MENUITEMINFO item_info = { sizeof(item_info), MIIM_STRING };
	#pragma clang diagnostic pop

	std::vector<wchar_t> buf(new_text.begin(), new_text.end());
	buf.push_back(0); // Null terminator
	item_info.dwTypeData = buf.data();
	SetMenuItemInfo(run.tray_popup, item, false, &item_info);
}

void RefreshMenu()
{
	Autostart::StartupState s_state = Autostart::GetStartupState();

	// This block of CheckPopupRadioItem might throw, but if that happens we just need to update the map, or something really fucked up happened
	CheckPopupRadioItem(IDM_BLUR, IDM_FLUENT, Tray::NORMAL_BUTTON_MAP.at(opt.taskbar_appearance));
	CheckPopupRadioItem(IDM_DYNAMICWS_BLUR, IDM_DYNAMICWS_CLEAR, Tray::DYNAMIC_BUTTON_MAP.at(opt.dynamic_ws_taskbar_appearance));
	CheckPopupRadioItem(IDM_PEEK, IDM_NOPEEK, Tray::PEEK_BUTTON_MAP.at(opt.peek));

	for (const std::pair<const swca::ACCENT, uint32_t> &kvp : Tray::DYNAMIC_BUTTON_MAP)
	{
		EnablePopupItem(kvp.second, opt.dynamic_ws);
	}
	EnablePopupItem(IDM_DYNAMICWS_PEEK, opt.dynamic_ws);
	EnablePopupItem(IDM_DYNAMICWS_COLOR, opt.dynamic_ws);
	EnablePopupItem(IDM_FLUENT, run.fluent_available);
	EnablePopupItem(IDM_DYNAMICWS_FLUENT, opt.dynamic_ws && run.fluent_available);
	EnablePopupItem(IDM_AUTOSTART, !(s_state == Autostart::StartupState::DisabledByUser || s_state == Autostart::StartupState::DisabledByPolicy));

	if (s_state == Autostart::StartupState::DisabledByUser)
	{
		ChangePopupItemText(IDM_AUTOSTART, L"Startup has been disabled in Task Manager");
	}
	else if (s_state == Autostart::StartupState::DisabledByPolicy)
	{
		ChangePopupItemText(IDM_AUTOSTART, L"Startup has been disabled in Group Policy");
	}
	else
	{
		ChangePopupItemText(IDM_AUTOSTART, L"Open at boot");
	}

	CheckPopupItem(IDM_DYNAMICWS_PEEK, opt.dynamic_ws_normal_on_peek);
	CheckPopupItem(IDM_DYNAMICWS, opt.dynamic_ws);
	CheckPopupItem(IDM_DYNAMICSTART, opt.dynamic_start);
	CheckPopupItem(IDM_AUTOSTART, s_state == Autostart::StartupState::Enabled);
	CheckPopupItem(IDM_VERBOSE, Config::VERBOSE);
}

void RegisterTray()
{
	Shell_NotifyIcon(NIM_ADD, &run.tray);
	Shell_NotifyIcon(NIM_SETVERSION, &run.tray);
}

LRESULT CALLBACK TrayCallback(const HWND hWnd, const uint32_t message, const WPARAM wParam, const LPARAM lParam)
{
	if (message == WM_CLOSE)
	{
		run.is_running = false;
		return 0;
	}
	else if (message == Tray::WM_NOTIFY_TB)
	{
		if (lParam == WM_LBUTTONUP || lParam == WM_RBUTTONUP)
		{
			static bool picker_open = false;
			RefreshMenu();
			POINT pt;
			GetCursorPos(&pt);
			SetForegroundWindow(hWnd);
			uint32_t tray = TrackPopupMenu(GetSubMenu(run.tray_popup, 0), TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, pt.x, pt.y, 0, hWnd, NULL);
			switch (tray)
			{
			case IDM_COLOR:
				if (picker_open)
				{
					break;
				}
				picker_open = true;
				opt.color = Util::PickColor(opt.color);
				picker_open = false;
				break;
			case IDM_NORMAL:
				opt.taskbar_appearance = swca::ACCENT_NORMAL;
				break;
			case IDM_CLEAR:
				opt.taskbar_appearance = swca::ACCENT_ENABLE_TRANSPARENTGRADIENT;
				break;
			case IDM_OPAQUE:
				opt.taskbar_appearance = swca::ACCENT_ENABLE_GRADIENT;
				break;
			case IDM_BLUR:
				opt.taskbar_appearance = swca::ACCENT_ENABLE_BLURBEHIND;
				break;
			case IDM_FLUENT:
				opt.taskbar_appearance = swca::ACCENT_ENABLE_FLUENT;
				break;
			case IDM_DYNAMICWS:
				opt.dynamic_ws = !opt.dynamic_ws;
				break;
			case IDM_DYNAMICWS_PEEK:
				opt.dynamic_ws_normal_on_peek = !opt.dynamic_ws_normal_on_peek;
				break;
			case IDM_DYNAMICWS_COLOR:
				if (picker_open)
				{
					break;
				}
				picker_open = true;
				opt.dynamic_ws_color = Util::PickColor(opt.dynamic_ws_color);
				picker_open = false;
				break;
			case IDM_DYNAMICWS_NORMAL:
				opt.dynamic_ws_taskbar_appearance = swca::ACCENT_NORMAL;
				break;
			case IDM_DYNAMICWS_CLEAR:
				opt.dynamic_ws_taskbar_appearance = swca::ACCENT_ENABLE_TRANSPARENTGRADIENT;
				break;
			case IDM_DYNAMICWS_OPAQUE:
				opt.dynamic_ws_taskbar_appearance = swca::ACCENT_ENABLE_GRADIENT;
				break;
			case IDM_DYNAMICWS_BLUR:
				opt.dynamic_ws_taskbar_appearance = swca::ACCENT_ENABLE_BLURBEHIND;
				break;
			case IDM_DYNAMICWS_FLUENT:
				opt.dynamic_ws_taskbar_appearance = swca::ACCENT_ENABLE_FLUENT;
				break;
			case IDM_DYNAMICSTART:
				opt.dynamic_start = !opt.dynamic_start;
				break;
			case IDM_PEEK:
				opt.peek = Taskbar::AEROPEEK::Enabled;
				break;
			case IDM_DPEEK:
				opt.peek = Taskbar::AEROPEEK::Dynamic;
				break;
			case IDM_NOPEEK:
				opt.peek = Taskbar::AEROPEEK::Disabled;
				break;
			case IDM_OPENLOG:
				Util::EditFile(Log::File);
				break;
			case IDM_VERBOSE:
				Config::VERBOSE = !Config::VERBOSE;
				break;
			case IDM_CLEARBLACKLISTCACHE:
				ClearBlacklistCache();
				break;
			case IDM_RELOADSETTINGS:
				ParseConfigFile();
				break;
			case IDM_EDITSETTINGS:
				SaveConfigFile();
				Util::EditFile(run.config_file);
				ParseConfigFile();
				break;
			case IDM_RETURNTODEFAULTSETTINGS:
				ApplyStock(Config::CONFIG_FILE);
				ParseConfigFile();
				break;
			case IDM_RELOADDYNAMICBLACKLIST:
				ParseBlacklistFile();
				ClearBlacklistCache();
				break;
			case IDM_EDITDYNAMICBLACKLIST:
				Util::EditFile(run.exclude_file);
				ParseBlacklistFile();
				ClearBlacklistCache();
				break;
			case IDM_RETURNTODEFAULTBLACKLIST:
				ApplyStock(Config::EXCLUDE_FILE);
				ParseBlacklistFile();
				ClearBlacklistCache();
				break;
			case IDM_AUTOSTART:
				Autostart::SetStartupState(Autostart::GetStartupState() == Autostart::StartupState::Enabled ? Autostart::StartupState::Disabled : Autostart::StartupState::Enabled);
				break;
			case IDM_EXITWITHOUTSAVING:
				run.exit_reason = Tray::UserActionNoSave;
				run.is_running = false;
				break;
			case IDM_EXIT:
				run.is_running = false;
				break;
			}
		}
		return 0;
	}
	else if (message == Tray::WM_TASKBARCREATED)
	{
		RefreshHandles();
		RegisterTray();
		return 0;
	}
	else if (message == WM_DISPLAYCHANGE)
	{
		RefreshHandles();
		return 0;
	}
	else if (message == Tray::NEW_TTB_INSTANCE)
	{
		run.exit_reason = Tray::NewInstance;
		run.is_running = false;
		return 0;
	}
	else
	{
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}

#pragma endregion

#pragma region Main logic

BOOL CALLBACK EnumWindowsProcess(const HWND hWnd, LPARAM)
{
	// IsWindowCloaked should take care of checking if it's on the current desktop.
	// But that's undefined behavior.
	// So eh, do both but with IsWindowOnCurrentDesktop last.
	if (IsWindowVisible(hWnd) && WindowHelper::IsWindowMaximised(hWnd) && !WindowHelper::IsWindowCloaked(hWnd) && !IsWindowBlacklisted(hWnd) && WindowHelper::IsWindowOnCurrentDesktop(hWnd))
	{
		HMONITOR _monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTOPRIMARY);
		Taskbar::TASKBARPROPERTIES &taskbar = run.taskbars.at(_monitor);
		if (opt.dynamic_ws)
		{
			taskbar.state = Taskbar::WindowMaximised;
		}

		if (opt.peek == Taskbar::AEROPEEK::Dynamic && taskbar.hwnd == run.main_taskbar)
		{
			run.should_show_peek = true;
		}
	}
	return true;
}

void CALLBACK HandleAeroPeekEvent(HWINEVENTHOOK, const DWORD event, HWND, LONG, LONG, DWORD, DWORD)
{
	run.peek_active = event == 0x21;
}

void SetTaskbarBlur()
{
	static int counter = 10;

	if (counter >= 10)	// Change this if you want to change the time it takes for the program to update.
	{					// 1 = opt.sleep_time; we use 10 (assuming the default opt.sleep_time value),
						// because the difference is less noticeable and it has no large impact on CPU.
						// We can change this if we feel that CPU is more important than response time.
		run.should_show_peek = (opt.peek == Taskbar::AEROPEEK::Enabled);

		for (std::pair<const HMONITOR, Taskbar::TASKBARPROPERTIES> &taskbar : run.taskbars)
		{
			taskbar.second.state = Taskbar::Normal; // Reset taskbar state
		}
		if (opt.dynamic_ws || opt.peek == Taskbar::AEROPEEK::Dynamic)
		{
			counter = 0;
			EnumWindows(&EnumWindowsProcess, NULL);
		}

		TogglePeek(run.should_show_peek);

		if (opt.dynamic_start)
		{
			static CComPtr<IAppVisibility> app_visibility;
			static bool failed = false;

			if (!app_visibility && !failed)
			{
				failed = !Error::Handle(app_visibility.CoCreateInstance(CLSID_AppVisibility), Error::Level::Log, L"Initialization of IAppVisibility failed.");
			}
			if (!failed && app_visibility)
			{
				BOOL start_visible;
				if (Error::Handle(app_visibility->IsLauncherVisible(&start_visible), Error::Level::Log, L"Checking start menu visibility failed.") && start_visible)
				{
					// TODO: does this works correctly on multi-monitor
					HWND start = FindWindow(L"Windows.UI.Core.CoreWindow", L"Start");
					run.taskbars.at(MonitorFromWindow(start, MONITOR_DEFAULTTOPRIMARY)).state = Taskbar::StartMenuOpen;
				}
			}
		}

		// TODO
		//if (true)
		//{
		//	HWND task_view = FindWindow(L"Windows.UI.Core.CoreWindow", L"Task view");
		//	if (task_view == GetForegroundWindow())
		//	{
		//		run.taskbars.at(MonitorFromWindow(task_view, MONITOR_DEFAULTTOPRIMARY)).state = Taskbar::StartMenuOpen;
		//	}
		//}

		if (opt.dynamic_ws && opt.dynamic_ws_normal_on_peek && run.peek_active)
		{
			for (std::pair<const HMONITOR, Taskbar::TASKBARPROPERTIES> &taskbar : run.taskbars)
			{
				taskbar.second.state = Taskbar::Normal;
			}
		}

		counter = 0;
	}

	for (const std::pair<const HMONITOR, Taskbar::TASKBARPROPERTIES> &taskbar : run.taskbars)
	{
		switch (taskbar.second.state)
		{
		case Taskbar::StartMenuOpen:
			SetWindowBlur(taskbar.second.hwnd, swca::ACCENT_NORMAL, NULL);
			break;
		case Taskbar::WindowMaximised:
			SetWindowBlur(taskbar.second.hwnd, opt.dynamic_ws_taskbar_appearance, opt.dynamic_ws_color); // A window is maximised; let's make sure that we blur the taskbar.
			break;
		case Taskbar::Normal:
			SetWindowBlur(taskbar.second.hwnd, opt.taskbar_appearance, opt.color);  // Taskbar should be normal, call using normal transparency settings
			break;
		}
	}
	counter++;
}

#pragma endregion

#pragma region Startup

void InitializeAPIs()
{
	static Microsoft::WRL::Wrappers::RoInitializeWrapper init(RO_INIT_SINGLETHREADED);
	Error::Handle(init, Error::Level::Log, L"Initialization of Windows Runtime failed.");
}

void InitializeTray(const HINSTANCE &hInstance)
{
	run.tray_popup = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_POPUP_MENU)); // Load our popup menu

	WNDCLASSEX wnd = {
		sizeof(wnd),							// cbSize
		CS_HREDRAW | CS_VREDRAW,				// style
		TrayCallback,							// lpfnWndProc
		NULL,									// cbClsExtra
		NULL,									// cbWndExtra
		hInstance,								// hInstance
		LoadIcon(NULL, IDI_APPLICATION),		// hIcon
		LoadCursor(NULL, IDC_ARROW),			// hCursor
		reinterpret_cast<HBRUSH>(BLACK_BRUSH),	// hbrBackground
		NULL,									// lpszMenuName
		App::NAME.c_str(),						// lpszClassName
		NULL									// hIconSm
	};

	RegisterClassEx(&wnd);

	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wmissing-field-initializers"
	run.tray = {
		sizeof(run.tray),																// cbSize
		CreateWindowEx(																	// hWnd
			WS_EX_TOOLWINDOW,																// dwExStyle
			App::NAME.c_str(),																// lpClassName
			L"TrayWindow",																	// lpWindowName
			WS_OVERLAPPEDWINDOW,															// dwStyle
			0,																				// x
			0,																				// y
			0,																				// nWidth
			0,																				// nHeight
			NULL,																			// hWndParent
			NULL,																			// hMenu
			hInstance,																		// hInstance
			NULL																			// lpParam
		),
		101,																			// uID
		NIF_ICON | NIF_TIP | NIF_MESSAGE,												// uFlags
		Tray::WM_NOTIFY_TB																// uCallbackMessage
	};
	LoadIconMetric(hInstance, MAKEINTRESOURCE(TRAYICON), LIM_LARGE, &run.tray.hIcon);	// hIcon
	wcscpy_s(run.tray.szTip, App::NAME.c_str());										// szTip
	#pragma clang diagnostic pop

	ShowWindow(run.tray.hWnd, WM_SHOWWINDOW);
	RegisterTray();
}

void Terminate()
{
	if (run.peek_hook)
	{
		UnhookWinEvent(run.peek_hook);
	}
	if (run.tray.cbSize)
	{
		Shell_NotifyIcon(NIM_DELETE, &run.tray);
	}
	if (Log::Instance)
	{
		delete Log::Instance;
	}
	exit(run.is_running ? 1 : 0);
}

int WINAPI WinMain(const HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
	// If there already is another instance running, tell it to exit
	if (!win32::IsSingleInstance())
	{
		HWND oldInstance = FindWindow(App::NAME.c_str(), L"TrayWindow");
		SendMessage(oldInstance, Tray::NEW_TTB_INSTANCE, NULL, NULL);
	}

	// Set our exit handler
	std::set_terminate(Terminate);

	// Initialize COM or UWP
	InitializeAPIs();

	// Get configuration file paths
	GetPaths();

	// If the configuration files don't exist, restore the files and show welcome to the users
	if (!CheckAndRunWelcome())
	{
		std::terminate();
	}

	StartLogger();

	// Verify our runtime
	run.fluent_available = win32::IsAtLeastBuild(17063);

	// Parse our configuration
	ParseConfigFile();
	ParseBlacklistFile();

	// Initialize GUI
	InitializeTray(hInstance);

	// Populate our vectors
	RefreshHandles();

	// Undoc'd, allows to detect when Aero Peek starts and stops
	run.peek_hook = SetWinEventHook(0x21, 0x22, NULL, HandleAeroPeekEvent, 0, 0, WINEVENT_OUTOFCONTEXT);

	// Message loop
	while (run.is_running)
	{
		MSG msg;
		if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		SetTaskbarBlur();
		std::this_thread::sleep_for(std::chrono::milliseconds(opt.sleep_time));
	}

	// If it's a new instance, don't save or restore taskbar to default
	if (run.exit_reason != Tray::NewInstance)
	{
		if (run.exit_reason != Tray::UserActionNoSave)
		{
			// Save configuration before we change opt
			SaveConfigFile();
		}

		// Restore default taskbar appearance
		opt.taskbar_appearance = swca::ACCENT_NORMAL;
		opt.peek = Taskbar::AEROPEEK::Enabled;
		opt.dynamic_start = false;
		opt.dynamic_ws = false;
		SetTaskbarBlur();
	}

	std::terminate();
}

#pragma endregion