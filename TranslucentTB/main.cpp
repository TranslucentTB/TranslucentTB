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
#include <PathCch.h>
#include <Psapi.h>
#include <shellapi.h>
#include <ShellScalingApi.h>
#include <ShlObj.h>
#include <wrl/wrappers/corewrappers.h>

// Local stuff
#include "app.hpp"
#include "autofree.hpp"
#include "autostart.hpp"
#include "common.h"
#include "config.hpp"
#include "eventhook.hpp"
#include "messagewindow.hpp"
#include "resource.h"
#include "swcadata.hpp"
#include "taskbar.hpp"
#include "tray.hpp"
#include "traycontextmenu.hpp"
#include "ttberror.hpp"
#include "ttblog.hpp"
#include "util.hpp"
#ifdef STORE
#include "UWP.hpp"
#endif
#include "win32.hpp"
#include "window.hpp"
#include "windowclass.hpp"


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
	Window main_taskbar;
	std::unordered_map<HMONITOR, Taskbar::TASKBARPROPERTIES> taskbars;
	bool should_show_peek;
	bool is_running = true;
	bool fluent_available = false;
	std::wstring config_folder;
	std::wstring config_file;
	std::wstring exclude_file;
	int cache_hits;
	bool peek_active = false;
} run;

#pragma endregion

#pragma region That one function that does all the magic

void SetWindowBlur(const Window &window, const swca::ACCENT &appearance, const uint32_t &color)
{
	if (user32::SetWindowCompositionAttribute)
	{
		static std::unordered_map<HWND, bool> is_normal;

		swca::ACCENTPOLICY policy = {
			appearance,
			2,
			(color & 0xFF00FF00) + ((color & 0x00FF0000) >> 16) + ((color & 0x000000FF) << 16),
			0
		};

		if (policy.nAccentState == swca::ACCENT_NORMAL)
		{
			if (is_normal.count(window) == 0 || !is_normal[window])
			{
				// WM_THEMECHANGED makes the taskbar reload the theme and reapply the normal effect.
				// Gotta memoize it because constantly sending it makes explorer's CPU usage jump.
				window.send_message(WM_THEMECHANGED);
				is_normal[window] = true;
			}
			return;
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

		user32::SetWindowCompositionAttribute(window, &data);
		is_normal[window] = false;
	}
}

#pragma endregion

#pragma region Configuration

void GetPaths()
{
#ifndef STORE
	AutoFree::CoTaskMem<wchar_t> appData;
	ErrorHandle(SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_DEFAULT, NULL, &appData), Error::Level::Fatal, L"Failed to determine configuration files locations!");
#else
	try
	{
		std::wstring appData_str = UWP::GetApplicationFolderPath(UWP::FolderType::Roaming);
		const wchar_t *appData = appData_str.c_str();
#endif

	AutoFree::Local<wchar_t> configFolder;
	AutoFree::Local<wchar_t> configFile;
	AutoFree::Local<wchar_t> excludeFile;

	ErrorHandle(PathAllocCombine(appData, App::NAME, PATHCCH_ALLOW_LONG_PATHS, &configFolder), Error::Level::Fatal, L"Failed to combine AppData folder and application name!");
	ErrorHandle(PathAllocCombine(configFolder, Config::CONFIG_FILE.c_str(), PATHCCH_ALLOW_LONG_PATHS, &configFile), Error::Level::Fatal, L"Failed to combine config folder and config file!");
	ErrorHandle(PathAllocCombine(configFolder, Config::EXCLUDE_FILE.c_str(), PATHCCH_ALLOW_LONG_PATHS, &excludeFile), Error::Level::Fatal, L"Failed to combine config folder and exclude file!");

	run.config_folder = configFolder;
	run.config_file = configFile;
	run.exclude_file = excludeFile;

#ifdef STORE
	}
	catch (const winrt::hresult_error &error)
	{
		ErrorHandle(error.code(), Error::Level::Fatal, L"Getting application folder paths failed!");
	}
#endif
}

void ApplyStock(const std::wstring &filename)
{
	std::wstring exeFolder_str = win32::GetExeLocation();
	exeFolder_str.erase(exeFolder_str.find_last_of(L"/\\") + 1);

	AutoFree::Local<wchar_t> stockFile;
	if (!ErrorHandle(PathAllocCombine(exeFolder_str.c_str(), filename.c_str(), PATHCCH_ALLOW_LONG_PATHS, &stockFile), Error::Level::Error, L"Failed to combine executable folder and config file!"))
	{
		return;
	}

	AutoFree::Local<wchar_t> configFile;
	if (!ErrorHandle(PathAllocCombine(run.config_folder.c_str(), filename.c_str(), PATHCCH_ALLOW_LONG_PATHS, &configFile), Error::Level::Error, L"Failed to combine config folder and config file!"))
	{
		return;
	}

	if (!win32::IsDirectory(run.config_folder))
	{
		if (!CreateDirectory(run.config_folder.c_str(), NULL))
		{
			if (!ErrorHandle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Error, L"Creating configuration files directory failed!"))
			{
				return;
			}
		}
	}

	if (!CopyFile(stockFile, configFile, FALSE))
	{
		ErrorHandle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Error, L"Copying stock configuration file failed!");
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

		if (MessageBox(NULL, message.c_str(), App::NAME, MB_ICONINFORMATION | MB_YESNO | MB_SETFOREGROUND) != IDYES)
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
			Log::OutputMessage(L"Invalid line in configuration file: " + line);
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
		configstream << right << setw(6) << setfill(L'0') << hex << (opt.color & 0x00FFFFFF);
		configstream << L" ; A color in hexadecimal notation." << endl;

		configstream << L"opacity=";
		configstream << left << setw(3) << setfill(L' ') << to_wstring((opt.color & 0xFF000000) >> 24);
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
	// Clear our vectors
	opt.blacklisted_classes.clear();
	opt.blacklisted_filenames.clear();
	opt.blacklisted_titles.clear();

	std::wifstream excludesfilestream(run.exclude_file);

	const wchar_t delimiter = L','; // Change to change the char(s) used to split,
	const wchar_t comment = L';';

	for (std::wstring line; std::getline(excludesfilestream, line);)
	{
		if (line.empty())
		{
			continue;
		}

		size_t comment_index = line.find(comment);
		if (comment_index == 0)
		{
			continue;
		}
		else if (comment_index != std::wstring::npos)
		{
			line = line.substr(0, comment_index);
		}

		if (line[line.length() - 1] != delimiter)
		{
			line += delimiter;
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

void RefreshHandles()
{
	if (Config::VERBOSE)
	{
		Log::OutputMessage(L"Refreshing taskbar handles");
	}

	// Older handles are invalid, so clear the map to be ready for new ones
	run.taskbars.clear();

	run.taskbars[run.main_taskbar.monitor()] = {
		run.main_taskbar = Window::Find(L"Shell_TrayWnd"),
		Taskbar::Normal
	};

	Window secondtaskbar;
	while ((secondtaskbar = Window::FindEx(nullptr, secondtaskbar, L"Shell_SecondaryTrayWnd")) != Window())
	{
		run.taskbars[secondtaskbar.monitor()] = { secondtaskbar, Taskbar::Normal };
	}
}

void TogglePeek(const bool &status)
{
	static bool cached_peek = true;
	static Window cached_taskbar = Window(run.main_taskbar);

	if (status != cached_peek || cached_taskbar != run.main_taskbar)
	{
		Window _tray = Window::FindEx(run.main_taskbar, nullptr, L"TrayNotifyWnd");
		Window _peek = Window::FindEx(_tray, nullptr, L"TrayShowDesktopButtonWClass");
		Window _overflow = Window::FindEx(_tray, nullptr, L"Button");

		_peek.show(status ? SW_SHOWNORMAL : SW_HIDE);

		// This is a really terrible hack, but it's the only way I found to make the changes reflect instantly.
		// Toggles the overflow area popup twice. Nearly imperceptible.
		// If you have a better solution, let me know or send a pull request
		_overflow.send_message(WM_LBUTTONUP);
		_overflow.send_message(WM_LBUTTONUP);

		cached_peek = status;
		cached_taskbar = Window(run.main_taskbar);
	}
}

void ClearBlacklistCache()
{
	run.cache_hits = Config::CACHE_HIT_MAX + 1;
}

bool OutputBlacklistMatchToLog(const Window &window, const bool &match)
{
	if (Config::VERBOSE)
	{
		std::wostringstream message;
		message << (match ? L"B" : L"No b") << L"lacklist match found for window: ";
		message << window.handle() << L" [" << window.classname() << L"] [" << window.filename() << L"] [" << window.title() << L"]";

		Log::OutputMessage(message.str());
	}

	return match;
}

bool IsWindowBlacklisted(const Window &window)
{
	static std::unordered_map<HWND, bool> blacklist_cache;

	if (run.cache_hits <= Config::CACHE_HIT_MAX && blacklist_cache.count(window) > 0)
	{
		run.cache_hits++;
		return blacklist_cache[window];
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
			for (const std::wstring &value : opt.blacklisted_classes)
			{
				if (window.classname() == value)
				{
					return OutputBlacklistMatchToLog(window, blacklist_cache[window] = true);
				}
			}
		}

		// Try it second because idk
		// Window names can change, but I don't think it will be a big issue if we cache it.
		// If it ends up affecting stuff, we can remove it from caching easily.
		if (opt.blacklisted_titles.size() > 0)
		{
			const std::wstring title = window.title();
			for (const std::wstring &value : opt.blacklisted_titles)
			{
				if (title.find(value) != std::wstring::npos)
				{
					return OutputBlacklistMatchToLog(window, blacklist_cache[window] = true);
				}
			}
		}

		// GetModuleFileNameEx is quite expensive according to the tracing tools, so use it as last resort.
		if (opt.blacklisted_filenames.size() > 0)
		{
			std::wstring exeName = window.filename();
			Util::ToLower(exeName);
			for (const std::wstring &value : opt.blacklisted_filenames)
			{
				if (exeName == value)
				{
					return OutputBlacklistMatchToLog(window, blacklist_cache[window] = true);
				}
			}
		}

		return OutputBlacklistMatchToLog(window, blacklist_cache[window] = false);
	}
}

#pragma endregion

#pragma region Tray

inline void ChangePopupItemText(HMENU menu, const uint32_t &item, const std::wstring &new_text)
{
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wmissing-field-initializers"
	MENUITEMINFO item_info = { sizeof(item_info), MIIM_STRING };
	#pragma clang diagnostic pop

	std::vector<wchar_t> buf(new_text.begin(), new_text.end());
	buf.push_back(0); // Null terminator
	item_info.dwTypeData = buf.data();
	SetMenuItemInfo(menu, item, false, &item_info);
}

void RefreshMenu(HMENU menu)
{
	Autostart::StartupState s_state = Autostart::GetStartupState();

	TrayContextMenu::RefreshBool(IDM_OPENLOG, menu, !Log::file().empty(), TrayContextMenu::ControlsEnabled);
	TrayContextMenu::RefreshBool(IDM_DYNAMICWS_FLUENT, menu, opt.dynamic_ws && run.fluent_available, TrayContextMenu::ControlsEnabled);
	TrayContextMenu::RefreshBool(IDM_AUTOSTART, menu, !(s_state == Autostart::StartupState::DisabledByUser
#ifdef STORE
		|| s_state == Autostart::StartupState::DisabledByPolicy
#endif
		), TrayContextMenu::ControlsEnabled);

	std::wstring autostart_text;
	switch (s_state)
	{
	case Autostart::StartupState::DisabledByUser:
		autostart_text = L"Startup has been disabled in Task Manager";
		break;
#ifdef STORE
	case Autostart::StartupState::DisabledByPolicy:
		autostart_text = L"Startup has been disabled in Group Policy";
		break;
#endif
	case Autostart::StartupState::Enabled:
	case Autostart::StartupState::Disabled:
		autostart_text = L"Open at boot";
	}
	ChangePopupItemText(menu, IDM_AUTOSTART, autostart_text);

	TrayContextMenu::RefreshBool(IDM_AUTOSTART, menu, s_state == Autostart::StartupState::Enabled, TrayContextMenu::Toggle);
}

#pragma endregion

#pragma region Main logic

BOOL CALLBACK EnumWindowsProcess(const HWND hWnd, LPARAM)
{
	const Window window = hWnd;
	// IsWindowCloaked should take care of checking if it's on the current desktop.
	// But that's undefined behavior.
	// So eh, do both but with IsWindowOnCurrentDesktop last.
	if (window.visible() && window.state() == SW_MAXIMIZE && !window.get_attribute<BOOL>(DWMWA_CLOAKED) && !IsWindowBlacklisted(window) && window.on_current_desktop())
	{
		Taskbar::TASKBARPROPERTIES &taskbar = run.taskbars.at(window.monitor());
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

		if (opt.dynamic_start && Util::IsStartVisible())
		{
			// TODO: does this works correctly on multi-monitor
			run.taskbars.at(Window::Find(L"Windows.UI.Core.CoreWindow", L"Start").monitor()).state = Taskbar::StartMenuOpen;
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

void InitializeWindowsRuntime()
{
	static Microsoft::WRL::Wrappers::RoInitializeWrapper init(RO_INIT_SINGLETHREADED);
	ErrorHandle(init, Error::Level::Log, L"Initialization of Windows Runtime failed.");
}

void HardenProcess()
{
	PROCESS_MITIGATION_ASLR_POLICY aslr_policy;
	if (GetProcessMitigationPolicy(GetCurrentProcess(), ProcessASLRPolicy, &aslr_policy, sizeof(aslr_policy)))
	{
		aslr_policy.EnableForceRelocateImages = true;
		aslr_policy.DisallowStrippedImages = true;
		if (!SetProcessMitigationPolicy(ProcessASLRPolicy, &aslr_policy, sizeof(aslr_policy)))
		{
			ErrorHandle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Log, L"Couldn't disallow stripped images.");
		}
	}
	else
	{
		ErrorHandle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Log, L"Couldn't get current ASLR policy.");
	}

	PROCESS_MITIGATION_DYNAMIC_CODE_POLICY code_policy {};
	code_policy.ProhibitDynamicCode = true;
	code_policy.AllowThreadOptOut = false;
	code_policy.AllowRemoteDowngrade = false;
	if (!SetProcessMitigationPolicy(ProcessDynamicCodePolicy, &code_policy, sizeof(code_policy)))
	{
		ErrorHandle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Log, L"Couldn't disable dynamic code generation.");
	}

	PROCESS_MITIGATION_STRICT_HANDLE_CHECK_POLICY handle_policy {};
	handle_policy.RaiseExceptionOnInvalidHandleReference = true;
	handle_policy.HandleExceptionsPermanentlyEnabled = true;
	if (!SetProcessMitigationPolicy(ProcessStrictHandleCheckPolicy, &handle_policy, sizeof(handle_policy)))
	{
		ErrorHandle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Log, L"Couldn't enable strict handle checks.");
	}

	PROCESS_MITIGATION_EXTENSION_POINT_DISABLE_POLICY extension_policy {};
	extension_policy.DisableExtensionPoints = true;
	if (!SetProcessMitigationPolicy(ProcessExtensionPointDisablePolicy, &extension_policy, sizeof(extension_policy)))
	{
		ErrorHandle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Log, L"Couldn't disable extension point DLLs.");
	}

	PROCESS_MITIGATION_BINARY_SIGNATURE_POLICY signature_policy {};
	signature_policy.MitigationOptIn = true;
	if (!SetProcessMitigationPolicy(ProcessSignaturePolicy, &signature_policy, sizeof(signature_policy)))
	{
		ErrorHandle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Log, L"Couldn't enable image signature enforcement.");
	}


	PROCESS_MITIGATION_IMAGE_LOAD_POLICY load_policy {};
	load_policy.NoLowMandatoryLabelImages = true;
	load_policy.PreferSystem32Images = true;

	// https://blogs.msdn.microsoft.com/oldnewthing/20160602-00/?p=93556
	std::vector<wchar_t> volumePath(LONG_PATH);
	if (GetVolumePathName(win32::GetExeLocation().c_str(), volumePath.data(), LONG_PATH))
	{
		load_policy.NoRemoteImages = GetDriveType(volumePath.data()) != DRIVE_REMOTE;
	}
	else
	{
		ErrorHandle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Log, L"Unable to get drive root.");
	}

	if (!SetProcessMitigationPolicy(ProcessImageLoadPolicy, &load_policy, sizeof(load_policy)))
	{
		ErrorHandle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Log, L"Couldn't set image load policy.");
	}
}

void InitializeTray(const HINSTANCE &hInstance)
{
	static MessageWindow window(L"TrayWindow", App::NAME, hInstance);
	static TrayContextMenu tray(window, MAKEINTRESOURCE(TRAYICON), MAKEINTRESOURCE(IDR_POPUP_MENU), hInstance);

	window.RegisterCallback(Tray::NEW_TTB_INSTANCE, [](Window, WPARAM, LPARAM) {
		run.exit_reason = Tray::NewInstance;
		run.is_running = false;
		return 0;
	});

	window.RegisterCallback(WM_DISPLAYCHANGE, [](Window, WPARAM, LPARAM) {
		RefreshHandles();
		return 0;
	});

	window.RegisterCallback(Tray::WM_TASKBARCREATED, [](Window, WPARAM, LPARAM) {
		RefreshHandles();
		return 0;
	});

	window.RegisterCallback(WM_CLOSE, [](Window, WPARAM, LPARAM) {
		run.exit_reason = Tray::UserAction;
		run.is_running = false;
		return 0;
	});

#ifdef STORE
	window.RegisterCallback(WM_QUERYENDSESSION, [](Window, WPARAM, LPARAM) {
		// https://docs.microsoft.com/en-us/windows/uwp/porting/desktop-to-uwp-extensions#updates
		RegisterApplicationRestart(NULL, NULL);
		return TRUE;
	});
#endif

	tray.BindEnum(IDM_BLUR, IDM_FLUENT, opt.taskbar_appearance, Tray::NORMAL_BUTTON_MAP);
	tray.BindEnum(IDM_DYNAMICWS_BLUR, IDM_DYNAMICWS_CLEAR, opt.dynamic_ws_taskbar_appearance, Tray::DYNAMIC_BUTTON_MAP);
	tray.BindEnum(IDM_PEEK, IDM_NOPEEK, opt.peek, Tray::PEEK_BUTTON_MAP);

	for (const auto &button_pair : Tray::DYNAMIC_BUTTON_MAP)
	{
		tray.BindBool(button_pair.second, opt.dynamic_ws, TrayContextMenu::ControlsEnabled);
	}

	tray.BindBool(IDM_DYNAMICWS_COLOR, opt.dynamic_ws,                TrayContextMenu::ControlsEnabled);
	tray.BindBool(IDM_DYNAMICWS_PEEK,  opt.dynamic_ws,                TrayContextMenu::ControlsEnabled);
	tray.BindBool(IDM_DYNAMICWS,       opt.dynamic_ws,                TrayContextMenu::Toggle);
	tray.BindBool(IDM_DYNAMICWS_PEEK,  opt.dynamic_ws_normal_on_peek, TrayContextMenu::Toggle);
	tray.BindBool(IDM_DYNAMICSTART,    opt.dynamic_start,             TrayContextMenu::Toggle);
	tray.BindBool(IDM_VERBOSE,         Config::VERBOSE,               TrayContextMenu::Toggle);
	tray.BindBool(IDM_FLUENT,          run.fluent_available,          TrayContextMenu::ControlsEnabled);

	tray.RegisterContextMenuCallback(IDM_EXITWITHOUTSAVING, [](unsigned int) {
		run.exit_reason = Tray::UserActionNoSave;
		run.is_running = false;
	});

	tray.RegisterContextMenuCallback(IDM_EXIT, [](unsigned int) {
		run.exit_reason = Tray::UserAction;
		run.is_running = false;
	});

	tray.RegisterContextMenuCallback(IDM_COLOR,           [](unsigned int) {
		Util::PickColor(opt.color);
	});
	tray.RegisterContextMenuCallback(IDM_DYNAMICWS_COLOR, [](unsigned int) {
		Util::PickColor(opt.dynamic_ws_color);
	});

	tray.RegisterCustomRefresh(RefreshMenu);

	tray.RegisterContextMenuCallback(IDM_OPENLOG, [](unsigned int) {
		Util::EditFile(Log::file());
	});

	tray.RegisterContextMenuCallback(IDM_CLEARBLACKLISTCACHE, [](unsigned int) {
		ClearBlacklistCache();
	});

	tray.RegisterContextMenuCallback(IDM_RELOADSETTINGS, [](unsigned int) {
		ParseConfigFile();
	});

	tray.RegisterContextMenuCallback(IDM_EDITSETTINGS, [](unsigned int) {
		SaveConfigFile();
		Util::EditFile(run.config_file);
		ParseConfigFile();
	});

	tray.RegisterContextMenuCallback(IDM_RETURNTODEFAULTSETTINGS, [](unsigned int) {
		ApplyStock(Config::CONFIG_FILE);
		ParseConfigFile();
	});

	tray.RegisterContextMenuCallback(IDM_RELOADDYNAMICBLACKLIST, [](unsigned int) {
		ParseBlacklistFile();
		ClearBlacklistCache();
	});

	tray.RegisterContextMenuCallback(IDM_EDITDYNAMICBLACKLIST, [](unsigned int) {
		Util::EditFile(run.exclude_file);
		ParseBlacklistFile();
		ClearBlacklistCache();
	});

	tray.RegisterContextMenuCallback(IDM_RETURNTODEFAULTBLACKLIST, [](unsigned int) {
		ApplyStock(Config::EXCLUDE_FILE);
		ParseBlacklistFile();
		ClearBlacklistCache();
	});

	tray.RegisterContextMenuCallback(IDM_AUTOSTART, [](unsigned int) {
		Autostart::SetStartupState(Autostart::GetStartupState() == Autostart::StartupState::Enabled ? Autostart::StartupState::Disabled : Autostart::StartupState::Enabled);
	});
}

void Terminate()
{
	exit(run.is_running ? EXIT_FAILURE : EXIT_SUCCESS);
}

int WINAPI wWinMain(const HINSTANCE hInstance, HINSTANCE, wchar_t *, int)
{
	HardenProcess();

	// If there already is another instance running, tell it to exit
	if (!win32::IsSingleInstance())
	{
		Window::Find(App::NAME, L"TrayWindow").send_message(Tray::NEW_TTB_INSTANCE);
	}

	// Set our exit handler
	std::set_terminate(Terminate);

	InitializeWindowsRuntime();

	// Get configuration file paths
	GetPaths();

	// If the configuration files don't exist, restore the files and show welcome to the users
	if (!CheckAndRunWelcome())
	{
		std::terminate();
	}

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
	// Marked as static because if we don't the destructor doesn't gets called when using exit()
	static EventHook peek_hook(0x21, 0x22, HandleAeroPeekEvent, WINEVENT_OUTOFCONTEXT);

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
			SaveConfigFile();
		}

		// Restore default taskbar appearance
		TogglePeek(true);
		for (const std::pair<const HMONITOR, Taskbar::TASKBARPROPERTIES> &taskbar : run.taskbars)
		{
			SetWindowBlur(taskbar.second.hwnd, swca::ACCENT_NORMAL, NULL);
		}
	}

	std::terminate();
}

#pragma endregion