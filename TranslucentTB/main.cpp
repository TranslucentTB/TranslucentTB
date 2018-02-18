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
#include <comdef.h>
#include <dwmapi.h>
#include <psapi.h>
#include <shellapi.h>
#include <ShellScalingAPI.h>
#include <ShlObj.h>

// UWP
#ifdef STORE
#include <roapi.h>
#endif

// For the context menu
#include "resource.h"

// For the color picker
#include "../CPicker/CPickerDll.h"

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


#pragma region Structures

static struct OPTIONS
{
	swca::ACCENT taskbar_appearance = swca::ACCENT_ENABLE_BLURBEHIND;
	uint32_t color = 0x00000000;
	bool dynamicws = false;
	swca::ACCENT dynamic_ws_state = swca::ACCENT_ENABLE_BLURBEHIND;	// State to activate when a window is maximised
	bool dynamicws_peek = true;										// Whether to use the normal style when using Aero Peek
	bool dynamicstart = false;
	uint32_t dynamicwscolor = 0x00000000;
	uint16_t sleep_time = 10;
	Taskbar::AEROPEEK peek = Taskbar::AEROPEEK::Enabled;
	std::vector<std::wstring> blacklisted_classes;
	std::vector<std::wstring> blacklisted_filenames;
	std::vector<std::wstring> blacklisted_titles;
} opt;

static struct RUNTIMESTATE
{
	Tray::EXITREASON exit_reason = Tray::UserAction;
	IVirtualDesktopManager *desktop_manager = NULL;				// Used to detect if a window is in the current virtual desktop. Don't forget to check for null on this one
	IAppVisibility *app_visibility = NULL;						// Used to detect if start menu is opened
	HWND main_taskbar;
	std::unordered_map<HMONITOR, Taskbar::TASKBARPROPERTIES> taskbars;
	bool should_show_peek;
	bool run = true;
	HMENU tray_popup;
	HANDLE app_handle;											// Handle to this app to check for uniqueness
	NOTIFYICONDATA tray;
	bool fluent_available = false;
	wchar_t config_folder[MAX_PATH];
	wchar_t config_file[MAX_PATH];
	wchar_t exclude_file[MAX_PATH];
	int cache_hits;
	bool peek_active = false;
	HWINEVENTHOOK peek_hook;
} run;

#pragma endregion

#pragma region That one function that does all the magic

void SetWindowBlur(const HWND &hWnd, const swca::ACCENT &appearance = swca::ACCENT_FOLLOW_OPT, const uint64_t &color = 0x100000000)
{
	if (user32::SetWindowCompositionAttribute)
	{
		uint32_t override_color = color == 0x100000000 ? opt.color : color;
		swca::ACCENTPOLICY policy = {
			appearance == swca::ACCENT_FOLLOW_OPT ? opt.taskbar_appearance : appearance,
			2,
			(override_color & 0xFF00FF00) + ((override_color & 0x00FF0000) >> 16) + ((override_color & 0x000000FF) << 16),
			0
		};

		// Handle fake values
		if (policy.nAccentState == swca::ACCENT_NORMAL)
		{
			policy.nAccentState = run.fluent_available ? swca::ACCENT_ENABLE_FLUENT : swca::ACCENT_ENABLE_TRANSPARENTGRADIENT;
			policy.nColor = 0x99000000;
		}

		// Fluent mode doesn't likes a completely 0 opacity
		if (policy.nAccentState == swca::ACCENT_ENABLE_FLUENT && policy.nColor >> 24 == 0x00)
		{
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
	LPWSTR localAppData;
	Error::Handle(SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_DEFAULT, NULL, &localAppData), Error::Level::Fatal, L"Failed to determine configuration files locations!");

	PathCombine(run.config_folder, localAppData, App::NAME);
	PathCombine(run.config_file, run.config_folder, Config::CONFIG_FILE);
	PathCombine(run.exclude_file, run.config_folder, Config::EXCLUDE_FILE);
}

void ApplyStock(const wchar_t *filename)
{
	wchar_t exeFolder[MAX_PATH];
	GetModuleFileName(GetModuleHandle(NULL), exeFolder, MAX_PATH);
	PathRemoveFileSpec(exeFolder);

	wchar_t stockFile[MAX_PATH];
	PathCombine(stockFile, exeFolder, filename);

	wchar_t configFile[MAX_PATH];
	PathCombine(configFile, run.config_folder, filename);

	if (!PathIsDirectory(run.config_folder))
	{
		if (!CreateDirectory(run.config_folder, NULL))
		{
			Error::Handle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Error, L"Creating configuration files directory failed!");
		}
	}

	if (!CopyFile(stockFile, configFile, FALSE))
	{
		Error::Handle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Error, L"Copying stock configuration file failed!");
	}
}

bool CheckAndRunWelcome()
{
	if (!PathIsDirectory(run.config_folder))
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

		if (MessageBox(NULL, message.c_str(), App::NAME, MB_ICONINFORMATION | MB_YESNO | MB_SETFOREGROUND) != IDYES)
		{
			return false;
		}
	}
	if (!PathFileExists(run.config_file))
	{
		ApplyStock(Config::CONFIG_FILE);
	}
	if (!PathFileExists(run.exclude_file))
	{
		ApplyStock(Config::EXCLUDE_FILE);
	}
	return true;
}

inline void UnknownValue(const std::wstring &key, const std::wstring &value)
{
	Log::OutputMessage(L"Unknown value found in configuration file: " + value + L" (for key: " + key + L")");
}

void ParseSingleConfigOption(std::wstring arg, std::wstring value)
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
		catch (const std::invalid_argument &e)
		{
			Log::OutputMessage(L"Could not parse sleep time found in configuration file: " + value);
		}
	}
	else if (arg == L"dynamic-ws")
	{
		if (value == L"true" || value == L"enable")
		{
			opt.dynamicws = true;
		}
		else if (value == L"blur")
		{
			opt.dynamicws = true;
			opt.dynamic_ws_state = swca::ACCENT_ENABLE_BLURBEHIND;
		}
		else if (value == L"opaque")
		{
			opt.dynamicws = true;
			opt.dynamic_ws_state = swca::ACCENT_ENABLE_GRADIENT;
		}
		else if (value == L"clear")
		{
			opt.dynamicws = true;
			opt.dynamic_ws_state = swca::ACCENT_ENABLE_TRANSPARENTGRADIENT;
		}
		else if (value == L"normal")
		{
			opt.dynamicws = true;
			opt.dynamic_ws_state = swca::ACCENT_NORMAL;
		}
		else if (value == L"fluent" && run.fluent_available)
		{
			opt.dynamicws = true;
			opt.dynamic_ws_state = swca::ACCENT_ENABLE_FLUENT;
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
			opt.dynamicstart = true;
		}
		else
		{
			UnknownValue(arg, value);
		}
	}
	else if (arg == L"color" || arg == L"tint")
	{
		std::wstring color_value = Util::Trim(value);

		if (color_value.find(L'#') == 0)
		{
			color_value = color_value.substr(1, color_value.length() - 1);
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
			opt.color = (opt.color & 0xFF000000) + (std::stoi(color_value, static_cast<size_t *>(0), 16) & 0x00FFFFFF);
		}
		catch (const std::invalid_argument &e)
		{
			Log::OutputMessage(L"Could not parse color found in configuration file: " + value + L" (after processing: " + color_value + L")");
		}
	}
	else if (arg == L"dynamic-ws-color")
	{
		std::wstring color_value = Util::Trim(value);

		if (color_value.find(L'#') == 0)
		{
			color_value = color_value.substr(1, color_value.length() - 1);
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
			opt.dynamicwscolor = (opt.dynamicwscolor & 0xFF000000) + (std::stoi(color_value, static_cast<size_t *>(0), 16) & 0x00FFFFFF);
		}
		catch (const std::invalid_argument &e)
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
		catch (const std::invalid_argument &e)
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
		catch (const std::invalid_argument &e)
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

		opt.dynamicwscolor = (parsed << 24) + (opt.dynamicwscolor & 0x00FFFFFF);
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
		else
		{
			UnknownValue(arg, value);
		}
	}
	else if (arg == L"dynamic-ws-normal-on-peek")
	{
		if (value == L"true" || value == L"enable")
		{
			opt.dynamicws_peek = true;
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
		else
		{
			UnknownValue(arg, value);
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
		default:
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

		if (!opt.dynamicws)
		{
			configstream << L"; ";
		}

		configstream << L"dynamic-ws=";
		switch (opt.dynamic_ws_state)
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
		default:
			configstream << L"enable";
			break;
		}
		configstream << endl;


		configstream << L"dynamic-ws-color=";
		configstream << right << setw(6) << setfill<wchar_t>('0') << hex << (opt.dynamicwscolor & 0x00FFFFFF);
		configstream << L" ; A color in hexadecimal notation." << endl;

		configstream << L"dynamic-ws-opacity=";
		configstream << left << setw(3) << setfill<wchar_t>(' ') << to_wstring((opt.dynamicwscolor & 0xFF000000) >> 24);
		configstream << L"  ; A value in the range 0 to 255." << endl;

		if (!opt.dynamicws_peek)
		{
			configstream << L"; ";
		}

		configstream << L"dynamic-ws-normal-on-peek=enable" << endl;

		if (!opt.dynamicstart)
		{
			configstream << L"; ";
		}

		configstream << L"dynamic-start=enable" << endl;

		configstream << endl;
		configstream << L"; Controls how the Aero Peek button behaves" << endl;
		configstream << L"peek=";
		switch (opt.peek)
		{
		case Taskbar::AEROPEEK::Disabled:
			configstream << L"hide";
			break;
		case Taskbar::AEROPEEK::Dynamic:
			configstream << L"dynamic";
			break;
		default:
			configstream << L"show";
			break;
		}
		configstream << endl;

		configstream << endl;
		configstream << L"; Advanced settings" << endl;
		configstream << L"; more informative logging. can make huge log files." << endl;

		if (!Config::VERBOSE)
		{
			configstream << L"; ";
		}

		configstream << L"verbose=enable" << endl;

		configstream << L"; sleep time in milliseconds, a shorter time reduces flicker when opening start, but results in higher CPU usage" << endl;
		configstream << L"sleep-time=" << opt.sleep_time << endl;
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
	wchar_t log_folder[MAX_PATH];

	PathCombine(log_folder, run.config_folder, L"Logs");

	if (!PathIsDirectory(log_folder))
	{
		if (!CreateDirectory(log_folder, NULL))
		{
			Error::Handle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Error, L"Creating log files directory failed!");
		}
	}

	wchar_t log_file[MAX_PATH];

	std::time_t unix_epoch = std::time(0);
	std::wstring log_filename = std::to_wstring(unix_epoch) + L".log";

	PathCombine(log_file, log_folder, log_filename.c_str());
	Log::Instance = new Logger(log_file);
	Log::File = log_file;
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

void TogglePeek(bool status)
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

std::wstring GetWindowTitle(HWND hwnd)
{
	int titleSize = GetWindowTextLength(hwnd) + 1; // For the null terminator
	std::vector<wchar_t> windowTitleBuffer(titleSize);
	GetWindowText(hwnd, windowTitleBuffer.data(), titleSize);
	return windowTitleBuffer.data();
}

void OutputBlacklistMatchToLog(HWND hwnd, bool match)
{
	if (Config::VERBOSE)
	{
		wchar_t className[MAX_PATH];
		GetClassName(hwnd, className, _countof(className));
		std::wstring classNameString(className);

		std::wstring title = GetWindowTitle(hwnd);

		DWORD ProcessId;
		GetWindowThreadProcessId(hwnd, &ProcessId);

		wchar_t exeName_path[MAX_PATH];
		GetModuleFileNameEx(OpenProcess(PROCESS_QUERY_INFORMATION, false, ProcessId), NULL, exeName_path, _countof(exeName_path));

		std::wstring exeName = PathFindFileName(exeName_path);

		std::wstringstream message;
		message << (match ? L"Blacklist match found for window: " : L"No blacklist match found for window: ");
		message << hwnd << L" [" << classNameString << L"] [" << exeName << L"] [" << title << L"]";

		Log::OutputMessage(message.str());
	}
}

bool IsWindowBlacklisted(HWND hWnd)
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
			wchar_t className[MAX_PATH];
			GetClassName(hWnd, className, _countof(className));
			std::wstring classNameString(className);
			for (const std::wstring &value : opt.blacklisted_classes)
			{
				if (classNameString == value)
				{
					OutputBlacklistMatchToLog(hWnd, true);
					return blacklist_cache[hWnd] = true;
				}
			}
		}

		// Try it second because idk
		// Window names can change, but I don't think it will be a big issue if we cache it.
		// If it ends up affecting stuff, we can remove it from caching easily.
		if (opt.blacklisted_titles.size() > 0)
		{
			std::wstring windowTitle = GetWindowTitle(hWnd);

			for (const std::wstring &value : opt.blacklisted_titles)
			{
				if (windowTitle.find(value) != std::wstring::npos)
				{
					OutputBlacklistMatchToLog(hWnd, true);
					return blacklist_cache[hWnd] = true;
				}
			}
		}

		// GetModuleFileNameEx is quite expensive according to the tracing tools, so use it as last resort.
		if (opt.blacklisted_filenames.size() > 0)
		{
			DWORD ProcessId;
			GetWindowThreadProcessId(hWnd, &ProcessId);

			wchar_t exeName_path[MAX_PATH];
			GetModuleFileNameEx(OpenProcess(PROCESS_QUERY_INFORMATION, false, ProcessId), NULL, exeName_path, _countof(exeName_path));

			std::wstring exeName = PathFindFileName(exeName_path);
			Util::ToLower(exeName);

			for (const std::wstring &value : opt.blacklisted_filenames)
			{
				if (exeName == value)
				{
					OutputBlacklistMatchToLog(hWnd, true);
					return blacklist_cache[hWnd] = true;
				}
			}
		}

		OutputBlacklistMatchToLog(hWnd, false);
		return blacklist_cache[hWnd] = false;
	}
}

bool IsWindowOnCurrentDesktop(HWND hWnd)
{
	BOOL on_current_desktop;  // This must be a BOOL not a bool because Windows and C89 are equally stupid
	return run.desktop_manager && SUCCEEDED(run.desktop_manager->IsWindowOnCurrentVirtualDesktop(hWnd, &on_current_desktop)) ? on_current_desktop : true;
}

bool IsWindowMaximised(HWND hWnd)
{
	WINDOWPLACEMENT result = {};
	GetWindowPlacement(hWnd, &result);
	return result.showCmd == SW_MAXIMIZE;
}

bool IsWindowCloaked(HWND hWnd)
{
	int cloaked;
	DwmGetWindowAttribute(hWnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked));
	return cloaked;
}

bool IsSingleInstance()
{
	run.app_handle = CreateEvent(NULL, TRUE, FALSE, App::ID);
	LRESULT error = GetLastError();
	switch (error)
	{
		case ERROR_ALREADY_EXISTS:
		{
			return false;
		}

		case ERROR_SUCCESS:
		{
			return true;
		}

		default:
		{
			Error::Handle(HRESULT_FROM_WIN32(error), Error::Level::Fatal, L"Failed to open app handle!");
			return true;
		}
	}
}

uint32_t PickColor(uint32_t color)
{
	unsigned short a;
	float alphaPercent;

	a = (color & 0xFF000000) >> 24;
	alphaPercent = a / 255.0f;
	a = static_cast<unsigned short>(std::round(alphaPercent * 100));

	unsigned short r = (color & 0x00FF0000) >> 16;
	unsigned short g = (color & 0x0000FF00) >> 8;
	unsigned short b = (color & 0x000000FF);

	// Bet 5 bucks a british wrote this library
	CColourPicker picker(NULL, r, g, b, a, true);
	picker.CreateColourPicker();
	SColour newColor = picker.GetCurrentColour();

	alphaPercent = newColor.a / 100.0f;
	a = static_cast<unsigned short>(std::round(alphaPercent * 255));

	return (a << 24) + (newColor.r << 16) + (newColor.g << 8) + newColor.b;
}

#pragma endregion

#pragma region Tray

DWORD CheckPopupItem(uint32_t item_to_check, bool state)
{
	return CheckMenuItem(run.tray_popup, item_to_check, MF_BYCOMMAND | (state ? MF_CHECKED : MF_UNCHECKED) | MF_ENABLED);
}

bool EnablePopupItem(uint32_t item_to_enable, bool state)
{
	return EnableMenuItem(run.tray_popup, item_to_enable, MF_BYCOMMAND | (state ? MF_ENABLED : MF_GRAYED));
}

bool CheckPopupRadioItem(uint32_t from, uint32_t to, uint32_t item_to_check)
{
	return CheckMenuRadioItem(run.tray_popup, from, to, item_to_check, MF_BYCOMMAND);
}

void RefreshMenu()
{
	Autostart::StartupState s_state = Autostart::GetStartupState();

	// This block of CheckPopupRadioItem might throw, but if that happens we just need to update the map, or something really fucked up happened
	CheckPopupRadioItem(IDM_BLUR, IDM_FLUENT, Tray::NORMAL_BUTTON_MAP.at(opt.taskbar_appearance));
	CheckPopupRadioItem(IDM_DYNAMICWS_BLUR, IDM_DYNAMICWS_CLEAR, Tray::DYNAMIC_BUTTON_MAP.at(opt.dynamic_ws_state));
	CheckPopupRadioItem(IDM_PEEK, IDM_NOPEEK, Tray::PEEK_BUTTON_MAP.at(opt.peek));

	for (const std::pair<swca::ACCENT, uint32_t> &kvp : Tray::DYNAMIC_BUTTON_MAP)
	{
		EnablePopupItem(kvp.second, opt.dynamicws);
	}
	EnablePopupItem(IDM_DYNAMICWS_PEEK, opt.dynamicws);
	EnablePopupItem(IDM_DYNAMICWS_COLOR, opt.dynamicws);
	EnablePopupItem(IDM_FLUENT, run.fluent_available);
	EnablePopupItem(IDM_DYNAMICWS_FLUENT, opt.dynamicws && run.fluent_available);
	EnablePopupItem(IDM_AUTOSTART, s_state != Autostart::StartupState::DisabledByUser);
	if (s_state == Autostart::StartupState::DisabledByUser)
	{
		// Change text to tell it has been disabled in task manager
	}
	else
	{
		// Put normal text
	}

	CheckPopupItem(IDM_DYNAMICWS_PEEK, opt.dynamicws_peek);
	CheckPopupItem(IDM_DYNAMICWS, opt.dynamicws);
	CheckPopupItem(IDM_DYNAMICSTART, opt.dynamicstart);
	CheckPopupItem(IDM_AUTOSTART, s_state == Autostart::StartupState::Enabled);
	CheckPopupItem(IDM_VERBOSE, Config::VERBOSE);
}

void RegisterTray()
{
	Shell_NotifyIcon(NIM_ADD, &run.tray);
	Shell_NotifyIcon(NIM_SETVERSION, &run.tray);
}

LRESULT CALLBACK TrayCallback(HWND hWnd, uint32_t message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_CLOSE)
	{
		PostQuitMessage(0);
	}
	else if (message == Tray::WM_NOTIFY_TB)
	{
		if (lParam == WM_LBUTTONUP || lParam == WM_RBUTTONUP)
		{
			RefreshMenu();
			POINT pt;
			GetCursorPos(&pt);
			SetForegroundWindow(hWnd);
			uint32_t tray = TrackPopupMenu(GetSubMenu(run.tray_popup, 0), TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, pt.x, pt.y, 0, hWnd, NULL);
			switch (tray) // TODO: Add dynamic windows ACCENT_ENABLE_TRANSPARENT_GRADIENT
			{
			case IDM_COLOR:
				opt.color = PickColor(opt.color);
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
				opt.dynamicws = !opt.dynamicws;
				break;
			case IDM_DYNAMICWS_PEEK:
				opt.dynamicws_peek = !opt.dynamicws_peek;
				break;
			case IDM_DYNAMICWS_COLOR:
				opt.dynamicwscolor = PickColor(opt.dynamicwscolor);
				break;
			case IDM_DYNAMICWS_NORMAL:
				opt.dynamic_ws_state = swca::ACCENT_NORMAL;
				break;
			case IDM_DYNAMICWS_CLEAR:
				opt.dynamic_ws_state = swca::ACCENT_ENABLE_TRANSPARENTGRADIENT;
				break;
			case IDM_DYNAMICWS_OPAQUE:
				opt.dynamic_ws_state = swca::ACCENT_ENABLE_GRADIENT;
				break;
			case IDM_DYNAMICWS_BLUR:
				opt.dynamic_ws_state = swca::ACCENT_ENABLE_BLURBEHIND;
				break;
			case IDM_DYNAMICWS_FLUENT:
				opt.dynamic_ws_state = swca::ACCENT_ENABLE_FLUENT;
				break;
			case IDM_DYNAMICSTART:
				opt.dynamicstart = !opt.dynamicstart;
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
				run.run = false;
				break;
			case IDM_EXIT:
				run.run = false;
				break;
			}
		}
	}
	else if (message == Tray::WM_TASKBARCREATED)
	{
		RefreshHandles();
		RegisterTray();
	}
	else if (message == Tray::NEW_TTB_INSTANCE)
	{
		run.exit_reason = Tray::NewInstance;
		run.run = false;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

#pragma endregion

#pragma region Main logic

BOOL CALLBACK EnumWindowsProcess(HWND hWnd, LPARAM)
{
	// IsWindowCloaked should take care of checking if it's on the current desktop.
	// But that's undefined behavior.
	// So eh, do both but with IsWindowOnCurrentDesktop last.
	if (IsWindowVisible(hWnd) && IsWindowMaximised(hWnd) && !IsWindowCloaked(hWnd) && !IsWindowBlacklisted(hWnd) && IsWindowOnCurrentDesktop(hWnd))
	{
		HMONITOR _monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTOPRIMARY);
		Taskbar::TASKBARPROPERTIES &taskbar = run.taskbars.at(_monitor);
		if (opt.dynamicws)
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

void CALLBACK HandleAeroPeekEvent(HWINEVENTHOOK, DWORD event, HWND, LONG, LONG, DWORD, DWORD)
{
	run.peek_active = event == 0x21;
}

void SetTaskbarBlur()
{
	static int counter = 0;

	if (counter >= 10)	// Change this if you want to change the time it takes for the program to update
	{					// 100 = 1 second; we use 10, because the difference is less noticeable and it has
						// no large impact on CPU. We can change this if we feel that CPU is more important
						// than response time.
		run.should_show_peek = (opt.peek == Taskbar::AEROPEEK::Enabled);

		for (std::pair<const HMONITOR, Taskbar::TASKBARPROPERTIES> &taskbar : run.taskbars)
		{
			taskbar.second.state = Taskbar::Normal; // Reset taskbar state
		}
		if (opt.dynamicws || opt.peek == Taskbar::AEROPEEK::Dynamic)
		{
			counter = 0;
			EnumWindows(&EnumWindowsProcess, NULL);
		}

		TogglePeek(run.should_show_peek);

		if (opt.dynamicstart)
		{
			BOOL start_visible;
			if (run.app_visibility && SUCCEEDED(run.app_visibility->IsLauncherVisible(&start_visible)) && start_visible)
			{
				// TODO: does this works correctly most of the time? (especially multi-monitor)
				// Is a window caption of "Start" reliable to check for (does it works on other UI cultures?)
				HWND start = FindWindow(L"Windows.UI.Core.CoreWindow", L"Start");
				run.taskbars.at(MonitorFromWindow(start, MONITOR_DEFAULTTOPRIMARY)).state = Taskbar::StartMenuOpen;
			}
		}

		//if (true)
		//{
		//	HWND task_view = FindWindow(L"Windows.UI.Core.CoreWindow", L"Task view");
		//	if (task_view == GetForegroundWindow())
		//	{
		//		run.taskbars.at(MonitorFromWindow(task_view, MONITOR_DEFAULTTOPRIMARY)).state = Taskbar::StartMenuOpen;
		//	}
		//}

		if (opt.dynamicws && opt.dynamicws_peek && run.peek_active)
		{
			for (std::pair<const HMONITOR, Taskbar::TASKBARPROPERTIES> &taskbar : run.taskbars)
			{
				taskbar.second.state = Taskbar::Normal;
			}
		}
	}

	for (const std::pair<HMONITOR, Taskbar::TASKBARPROPERTIES> &taskbar : run.taskbars)
	{
		switch (taskbar.second.state)
		{
		case Taskbar::StartMenuOpen:
			SetWindowBlur(taskbar.second.hwnd, swca::ACCENT_NORMAL);
			break;
		case Taskbar::WindowMaximised:
			SetWindowBlur(taskbar.second.hwnd, opt.dynamic_ws_state, opt.dynamicwscolor); // A window is maximised; let's make sure that we blur the taskbar.
			break;
		case Taskbar::Normal:
			SetWindowBlur(taskbar.second.hwnd);  // Taskbar should be normal, call using normal transparency settings
			break;
		}
	}
	counter++;
}

#pragma endregion

#pragma region Startup

void InitializeAPIs()
{
#ifdef STORE
	Error::Handle(ABI::Windows::Foundation::Initialize(), Error::Level::Log, L"Initialization of UWP failed.");
#endif
	Error::Handle(CoInitialize(NULL), Error::Level::Log, L"Initialization of COM failed.");
	Error::Handle(CoCreateInstance(CLSID_VirtualDesktopManager, NULL, CLSCTX_INPROC_SERVER, IID_IVirtualDesktopManager, reinterpret_cast<LPVOID *>(&run.desktop_manager)), Error::Level::Log, L"Initialization of IVirtualDesktopManager failed.");
	Error::Handle(CoCreateInstance(CLSID_AppVisibility, NULL, CLSCTX_INPROC_SERVER, IID_IAppVisibility, reinterpret_cast<LPVOID *>(&run.app_visibility)), Error::Level::Log, L"Initialization of IAppVisibility failed.");
}

void UninitializeAPIs()
{
	CoUninitialize();
#ifdef STORE
	ABI::Windows::Foundation::Uninitialize();
#endif
}

void InitializeTray(HINSTANCE hInstance)
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
		App::NAME,								// lpszClassName
		NULL									// hIconSm
	};

	RegisterClassEx(&wnd);

	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wmissing-field-initializers"
	run.tray = {
		sizeof(run.tray),																// cbSize
		CreateWindowEx(																	// hWnd
			WS_EX_TOOLWINDOW,															// dwExStyle
			App::NAME,																    // lpClassName
			L"TrayWindow",																// lpWindowName
			WS_OVERLAPPEDWINDOW,														// dwStyle
			0,																			// x
			0,																			// y
			0,																			// nWidth
			0,																			// nHeight
			NULL,																		// hWndParent
			NULL,																		// hMenu
			hInstance,																	// hInstance
			NULL																		// lpParam
		),
		101,																			// uID
		NIF_ICON | NIF_TIP | NIF_MESSAGE,												// uFlags
		Tray::WM_NOTIFY_TB																// uCallbackMessage
	};
	LoadIconMetric(hInstance, MAKEINTRESOURCE(MAINICON), LIM_LARGE, &run.tray.hIcon);	// hIcon
	wcscpy_s(run.tray.szTip, App::NAME);												// szTip
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
	UninitializeAPIs();
	if (run.tray.cbSize)
	{
		Shell_NotifyIcon(NIM_DELETE, &run.tray);
	}
	if (run.app_handle)
	{
		CloseHandle(run.app_handle);
	}
	if (Log::Instance)
	{
		delete Log::Instance;
	}
	exit(run.run ? 1 : 0);
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int)
{
	// If there already is another instance running, tell it to exit
	if (!IsSingleInstance())
	{
		HWND oldInstance = FindWindow(App::NAME, L"TrayWindow");
		SendMessage(oldInstance, Tray::NEW_TTB_INSTANCE, NULL, NULL);
	}

	// Set our exit handler
	std::set_terminate(Terminate);

	// Get configuration file paths
	GetPaths();

	// Set DPI awareness before showing the welcome dialog
	if (!SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
	{
		Error::Handle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Log, L"Setting DPI awareness failed.");
	}

	// If the configuration files don't exist, restore the files and show welcome to the users
	if (!CheckAndRunWelcome())
	{
		std::terminate();
	}

	StartLogger();

	// Initialize COM, UWP, and acquire a VirtualDesktopManager and AppVisibility interface
	InitializeAPIs();

	// Verify our runtime
	run.fluent_available = win32::IsAtLeastBuild(17063);

	// Parse our configuration
	ParseConfigFile();
	ParseBlacklistFile();

	// Initialize GUI
	InitializeTray(hInstance);

	// Populate our vectors
	RefreshHandles();

	// Scan windows to start taskbar with the correct mode immediatly
	if (opt.dynamicws || opt.peek == Taskbar::AEROPEEK::Dynamic)
	{
		EnumWindows(&EnumWindowsProcess, NULL);
	}

	// Undoc'd, allows to detect when Aero Peek starts and stops
	run.peek_hook = SetWinEventHook(0x21, 0x22, NULL, HandleAeroPeekEvent, 0, 0, WINEVENT_OUTOFCONTEXT);

	// Message loop
	while (run.run)
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
		opt.dynamicstart = false;
		opt.dynamicws = false;
		SetTaskbarBlur();
	}

	std::terminate();
	return 0;
}

#pragma endregion