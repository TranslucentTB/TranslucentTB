// Standard API
#include <cwchar>
#include <cwctype>
#include <fstream>
#include <iomanip>
#include <string>
#include <unordered_map>
#include <vector>

// Windows API
#include <comdef.h>
#include <dwmapi.h>
#include <psapi.h>
#include <roapi.h>
#include <shellapi.h>
#include <ShellScalingAPI.h>
#include <ShlObj.h>

// For the context menu
#include "resource.h"

// For the color picker
#include "../CPicker/CPicker.h"

#include "swcadata.hpp"
#include "config.hpp"
#include "taskbar.hpp"
#include "tray.hpp"
#include "util.hpp"
#include "win32.hpp"
#include "app.hpp"


#pragma region Structures

static struct OPTIONS
{
	swca::ACCENTSTATE taskbar_appearance = swca::ACCENT_ENABLE_BLURBEHIND;
	uint32_t color = 0x00000000;
	bool dynamicws = false;
	swca::ACCENTSTATE dynamic_ws_state = swca::ACCENT_ENABLE_BLURBEHIND;	// State to activate when a window is maximised
	bool dynamicws_peek = true;												// Whether to use the normal style when using Aero Peek
	bool dynamicstart = false;
	Taskbar::AEROPEEKSTATE peek = Taskbar::Enabled;
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
	std::unordered_map<HWND, Taskbar::TASKBARPROPERTIES> taskbars;
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
} run;

#pragma endregion

#pragma region That one function that does all the magic

void SetWindowBlur(HWND hWnd, swca::ACCENTSTATE appearance = swca::ACCENT_FOLLOW_OPT)
{
	if (user32::SetWindowCompositionAttribute)
	{
		swca::ACCENTPOLICY policy;
		uint32_t color = (opt.color & 0xFF00FF00) + ((opt.color & 0x00FF0000) >> 16) + ((opt.color & 0x000000FF) << 16);

		if (appearance != swca::ACCENT_FOLLOW_OPT) // Custom taskbar appearance is set
		{
			if (appearance == swca::ACCENT_ENABLE_TINTED) // Window is maximised
			{
				policy = { swca::ACCENT_ENABLE_TRANSPARENTGRADIENT, 2, color, 0 };
			}
			else if (appearance == swca::ACCENT_NORMAL)
			{
				policy = { (run.fluent_available ? swca::ACCENT_ENABLE_FLUENT : swca::ACCENT_ENABLE_TRANSPARENTGRADIENT), 2, 0x99000000, 0 };
			}
			else
			{
				policy = { appearance, 2, opt.color, 0 };
			}
		}
		else // Use the defaults
		{
			if (opt.dynamic_ws_state == swca::ACCENT_ENABLE_TINTED) // dynamic-ws is tint and desktop is shown
			{
				policy = { swca::ACCENT_ENABLE_TRANSPARENTGRADIENT, 2, 0x00000000, 0 };
			}
			else if (opt.taskbar_appearance == swca::ACCENT_NORMAL) // normal gradient color
			{
				policy = { (run.fluent_available ? swca::ACCENT_ENABLE_FLUENT : swca::ACCENT_ENABLE_TRANSPARENTGRADIENT), 2, 0x99000000, 0 };
			}
			else
			{
				policy = { opt.taskbar_appearance, 2, color, 0 };
			}
		}

		swca::WINCOMPATTRDATA data = { swca::WCA_ACCENT_POLICY, &policy, sizeof(policy) };
		user32::SetWindowCompositionAttribute(hWnd, &data);
	}
}

#pragma endregion

#pragma region Configuration

HRESULT GetPaths()
{
	LPWSTR localAppData;
	HRESULT error = SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_DEFAULT, NULL, &localAppData);

	if (FAILED(error))
	{
		return error;
	}

	PathCombine(run.config_folder, localAppData, App::NAME);
	PathCombine(run.config_file, run.config_folder, Config::CONFIG_FILE);
	PathCombine(run.exclude_file, run.config_folder, Config::EXCLUDE_FILE);

	return ERROR_SUCCESS;
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
		CreateDirectory(run.config_folder, NULL);
	}

	CopyFile(stockFile, configFile, FALSE);
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
		message += L"\n\nBy selecting OK and continuing, you agree to the GPLv3 license.";

		if (MessageBox(NULL, message.c_str(), App::NAME, MB_ICONINFORMATION | MB_OKCANCEL) != IDOK)
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
	}
	else if (arg == L"dynamic-ws")
	{
		if (value == L"true" || value == L"enable")
		{
			opt.dynamicws = true;
		}
		else if (value == L"tint")
		{
			opt.dynamicws = true;
			opt.dynamic_ws_state = swca::ACCENT_ENABLE_TINTED;
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
	}
	else if (arg == L"dynamic-start")
	{
		if (value == L"true" || value == L"enable")
		{
			opt.dynamicstart = true;
		}
	}
	else if (arg == L"color" || arg == L"tint")
	{
		value = Util::Trim(value);

		if (value.find(L'#') == 0)
		{
			value = value.substr(1, value.length() - 1);
		}

		// Get only the last 6 characters, keeps compatibility with old version.
		// It stored AARRGGBB in color, but now we store it as RRGGBB.
		// We read AA from opacity instead, which the old version also saved alpha to.
		if (value.length() > 6)
		{
			value = value.substr(value.length() - 6, 6);
		}

		opt.color = std::stoi(value, static_cast<size_t *>(0), 16);
	}
	else if (arg == L"opacity")
	{
		int parsed = std::stoi(value);

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
	else if (arg == L"peek")
	{
		if (value == L"hide")
		{
			opt.peek = Taskbar::Disabled;
		}
		else if (value == L"dynamic")
		{
			opt.peek = Taskbar::Dynamic;
		}
	}
	else if (arg == L"dynamic-ws-normal-on-peek")
	{
		if (value == L"true" || value == L"enable")
		{
			opt.dynamicws_peek = true;
		}
	}
}

void ParseConfigFile()
{
	std::wifstream configstream(run.config_file);

	for (std::wstring line; std::getline(configstream, line); )
	{
		// Skip comments
		size_t comment_index = line.find(L';');
		if (comment_index == 0)
		{
			continue;
		}
		else
		{
			line = line.substr(0, comment_index);
		}

		size_t split_index = line.find(L'=');
		std::wstring key = line.substr(0, split_index);
		std::wstring val = line.substr(split_index + 1, line.length() - split_index - 1);

		ParseSingleConfigOption(key, val);
	}
}

void SaveConfigFile()
{
	std::wstring configfile(run.config_file);
	if (!configfile.empty())
	{
		using namespace std;
		wofstream configstream(configfile);

		configstream << L"; Taskbar appearance: opaque, clear, normal, or blur (default)." << endl;

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

		configstream << endl;
		configstream << L"; Dynamic states: Window States and Start Menu" << endl;
		configstream << L"; dynamic windows: opaque, tint, normal, or blur (default)." << endl;
		configstream << L"; dynamic windows can be used in conjunction with a custom color and non-zero opacity!" << endl;
		configstream << L"; by enabling dynamic-ws-normal-on-peek, dynamic windows will return to the normal non-maximised state when using Aero Peek." << endl;
		configstream << L"; you can also set an accent value, which will represent the state of dynamic windows when there is no window maximised" << endl;

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
		case swca::ACCENT_ENABLE_TINTED:
			configstream << L"tint";
			break;
		case swca::ACCENT_ENABLE_GRADIENT:
			configstream << L"opaque";
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
		configstream << L"; Color and opacity of the taskbar." << endl;

		configstream << L"color=";
		configstream << right << setw(6) << setfill<wchar_t>('0') << hex << (opt.color & 0x00FFFFFF);
		configstream << L" ; A color in hexadecimal notation." << endl;

		configstream << L"opacity=";
		configstream << left << setw(3) << setfill<wchar_t>(' ') << to_wstring((opt.color & 0xFF000000) >> 24);
		configstream << L"  ; A value in the range 0 to 255." << endl;
		configstream << endl;

		configstream << L"; Controls how the Aero Peek button behaves" << endl;
		configstream << L"peek=";
		switch (opt.peek)
		{
		case Taskbar::Disabled:
			configstream << L"hide";
			break;
		case Taskbar::Dynamic:
			configstream << L"dynamic";
			break;
		default:
			configstream << L"show";
			break;
		}
		configstream << endl;
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
		size_t comment_index = line.find(L';');
		if (comment_index == 0)
		{
			continue;
		}
		else
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
	}
}

#pragma endregion

#pragma region Utilities

void RefreshHandles()
{
	HWND secondtaskbar = NULL;
	Taskbar::TASKBARPROPERTIES _properties;

	// Older handles are invalid, so clear the map to be ready for new ones
	run.taskbars.clear();

	run.main_taskbar = FindWindow(L"Shell_TrayWnd", NULL);
	_properties.hmon = MonitorFromWindow(run.main_taskbar, MONITOR_DEFAULTTOPRIMARY);
	_properties.state = Taskbar::Normal;
	run.taskbars.insert(std::make_pair(run.main_taskbar, _properties));

	while ((secondtaskbar = FindWindowEx(0, secondtaskbar, L"Shell_SecondaryTrayWnd", NULL)) != 0)
	{
		_properties.hmon = MonitorFromWindow(secondtaskbar, MONITOR_DEFAULTTOPRIMARY);
		_properties.state = Taskbar::Normal;
		run.taskbars.insert(std::make_pair(secondtaskbar, _properties));
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
					return blacklist_cache[hWnd] = true;
				}
			}
		}

		// Try it second because idk
		// Window names can change, but I don't think it will be a big issue if we cache it.
		// If it ends up affecting stuff, we can remove it from caching easily.
		if (opt.blacklisted_titles.size() > 0)
		{
			int titleSize = GetWindowTextLength(hWnd) + 1; // For the null terminator
			std::vector<wchar_t> windowTitleBuffer(titleSize);
			GetWindowText(hWnd, windowTitleBuffer.data(), titleSize);
			std::wstring windowTitle = windowTitleBuffer.data();

			for (const std::wstring &value : opt.blacklisted_titles)
			{
				if (windowTitle.find(value) != std::wstring::npos)
				{
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
					return blacklist_cache[hWnd] = true;
				}
			}
		}

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
	return GetLastError() != ERROR_ALREADY_EXISTS;
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
	// This block of CheckPopupRadioItem might throw, but if that happens we just need to update the map, or something really fucked up happened
	CheckPopupRadioItem(IDM_BLUR, IDM_FLUENT, Tray::NORMAL_BUTTON_MAP.at(opt.taskbar_appearance));
	CheckPopupRadioItem(IDM_DYNAMICWS_BLUR, IDM_DYNAMICWS_FLUENT, Tray::DYNAMIC_BUTTON_MAP.at(opt.dynamic_ws_state));
	CheckPopupRadioItem(IDM_PEEK, IDM_NOPEEK, Tray::PEEK_BUTTON_MAP.at(opt.peek));

	for (const uint32_t &item : { IDM_DYNAMICWS_BLUR, IDM_DYNAMICWS_CLEAR, IDM_DYNAMICWS_NORMAL, IDM_DYNAMICWS_OPAQUE, IDM_DYNAMICWS_PEEK })
		EnablePopupItem(item, opt.dynamicws);

	EnablePopupItem(IDM_FLUENT, run.fluent_available);
	EnablePopupItem(IDM_DYNAMICWS_FLUENT, opt.dynamicws && run.fluent_available);
	CheckPopupItem(IDM_DYNAMICWS_PEEK, opt.dynamicws_peek);
	CheckPopupItem(IDM_DYNAMICWS, opt.dynamicws);
	CheckPopupItem(IDM_DYNAMICSTART, opt.dynamicstart);
	CheckPopupItem(IDM_AUTOSTART, win32::GetStartupState());
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
			case IDM_BLUR:
				opt.taskbar_appearance = swca::ACCENT_ENABLE_BLURBEHIND;
				break;
			case IDM_CLEAR:
				opt.taskbar_appearance = swca::ACCENT_ENABLE_TRANSPARENTGRADIENT;
				break;
			case IDM_NORMAL:
				opt.taskbar_appearance = swca::ACCENT_NORMAL;
				break;
			case IDM_OPAQUE:
				opt.taskbar_appearance = swca::ACCENT_ENABLE_GRADIENT;
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
			case IDM_DYNAMICWS_BLUR:
				opt.dynamic_ws_state = swca::ACCENT_ENABLE_BLURBEHIND;
				break;
			case IDM_DYNAMICWS_CLEAR:
				opt.dynamic_ws_state = swca::ACCENT_ENABLE_TINTED;
				break;
			case IDM_DYNAMICWS_NORMAL:
				opt.dynamic_ws_state = swca::ACCENT_NORMAL;
				break;
			case IDM_DYNAMICWS_OPAQUE:
				opt.dynamic_ws_state = swca::ACCENT_ENABLE_GRADIENT;
				break;
			case IDM_DYNAMICWS_FLUENT:
				opt.dynamic_ws_state = swca::ACCENT_ENABLE_FLUENT;
				break;
			case IDM_DYNAMICSTART:
				opt.dynamicstart = !opt.dynamicstart;
				break;
			case IDM_COLOR:
			{
				unsigned short a;
				float alphaPercent;

				a = (opt.color & 0xFF000000) >> 24;
				alphaPercent = a / 255.0f;
				a = static_cast<unsigned short>(std::round(alphaPercent * 100));

				unsigned short r = (opt.color & 0x00FF0000) >> 16;
				unsigned short g = (opt.color & 0x0000FF00) >> 8;
				unsigned short b = (opt.color & 0x000000FF);

				// Bet 5 bucks a british wrote this library
				CColourPicker picker(NULL, r, g, b, a, true);
				picker.CreateColourPicker(CP_USE_ALPHA);
				SColour newColor = picker.GetCurrentColour();

				alphaPercent = newColor.a / 100.0f;
				a = static_cast<unsigned short>(std::round(alphaPercent * 255));

				opt.color = (a << 24) + (newColor.r << 16) + (newColor.g << 8) + newColor.b;
				break;
			}
			case IDM_PEEK:
				opt.peek = Taskbar::Enabled;
				break;
			case IDM_DPEEK:
				opt.peek = Taskbar::Dynamic;
				break;
			case IDM_NOPEEK:
				opt.peek = Taskbar::Disabled;
				break;
			case IDM_AUTOSTART: // TODO: Use UWP Apis
				win32::SetStartupState(!win32::GetStartupState());
				break;
			case IDM_RETURNTODEFAULTSETTINGS:
				ApplyStock(Config::CONFIG_FILE);
			case IDM_RELOADSETTINGS:
				ParseConfigFile();
				break;
			case IDM_EDITSETTINGS:
				SaveConfigFile();
				Util::EditFile(run.config_file);
				ParseConfigFile();
				break;
			case IDM_RETURNTODEFAULTBLACKLIST:
				ApplyStock(Config::EXCLUDE_FILE);
			case IDM_RELOADDYNAMICBLACKLIST:
				ParseBlacklistFile();
				ClearBlacklistCache();
				break;
			case IDM_EDITDYNAMICBLACKLIST:
				Util::EditFile(run.exclude_file);
				ParseBlacklistFile();
				ClearBlacklistCache();
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
	else if (message == Tray::NEW_TTB_INSTANCE) {
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
		for (std::pair<const HWND, Taskbar::TASKBARPROPERTIES> &taskbar : run.taskbars)
		{
			if (taskbar.second.hmon == _monitor)
			{
				if (opt.dynamicws)
				{
					taskbar.second.state = Taskbar::WindowMaximised;
				}

				if (opt.peek == Taskbar::Dynamic && taskbar.first == run.main_taskbar)
				{
					run.should_show_peek = true;
				}

				break;
			}
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
		run.should_show_peek = (opt.peek == Taskbar::Enabled);

		for (std::pair<const HWND, Taskbar::TASKBARPROPERTIES> &taskbar : run.taskbars)
		{
			taskbar.second.state = Taskbar::Normal; // Reset taskbar state
		}
		if (opt.dynamicws || opt.peek == Taskbar::Dynamic)
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
				// If not, is a window caption of "Start" reliable to check for (does it works on other UI cultures?)
				HWND start = FindWindow(L"Windows.UI.Core.CoreWindow", NULL);
				HMONITOR monitor = MonitorFromWindow(start, MONITOR_DEFAULTTOPRIMARY);
				for (std::pair<const HWND, Taskbar::TASKBARPROPERTIES> &taskbar : run.taskbars)
				{
					if (taskbar.second.hmon == monitor)
					{
						taskbar.second.state = Taskbar::StartMenuOpen;
						break; // Useless to continue looping, we found what we desired.
					}
				}
			}
		}

		if (opt.dynamicws && opt.dynamicws_peek && run.peek_active)
		{
			for (std::pair<const HWND, Taskbar::TASKBARPROPERTIES> &taskbar : run.taskbars)
			{
				taskbar.second.state = Taskbar::Normal;
			}
		}
	}

	for (const std::pair<HWND, Taskbar::TASKBARPROPERTIES> &taskbar : run.taskbars)
	{
		switch (taskbar.second.state)
		{
		case Taskbar::StartMenuOpen:
			SetWindowBlur(taskbar.first, swca::ACCENT_NORMAL);
			break;
		case Taskbar::WindowMaximised:
			SetWindowBlur(taskbar.first, opt.dynamic_ws_state); // A window is maximised; let's make sure that we blur the taskbar.
			break;
		case Taskbar::Normal:
			SetWindowBlur(taskbar.first);  // Taskbar should be normal, call using normal transparency settings
			break;
		}
	}
	counter++;
}

#pragma endregion

#pragma region Startup

void InitializeAPIs()
{
	HRESULT result;
	std::wstring buffer;

	if (!SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
	{
		result = GetLastError();
		buffer += L"Initialization of DPI failed. Exception from HRESULT: ";
		buffer += _com_error(result).ErrorMessage();
		buffer += '\n';
	}

	if (FAILED(result = Windows::Foundation::Initialize()))
	{
		buffer += L"Initialization of UWP failed. Exception from HRESULT: ";
		buffer += _com_error(result).ErrorMessage();
		buffer += '\n';
	}

	if (FAILED(result = CoInitialize(NULL)))
	{
		buffer += L"Initialization of COM failed. Exception from HRESULT: ";
		buffer += _com_error(result).ErrorMessage();
		buffer += '\n';
	}

	if (FAILED(result = CoCreateInstance(__uuidof(VirtualDesktopManager), NULL, CLSCTX_INPROC_SERVER, IID_IVirtualDesktopManager, reinterpret_cast<LPVOID *>(&run.desktop_manager))))
	{
		buffer += L"Initialization of VDM failed. Exception from HRESULT: ";
		buffer += _com_error(result).ErrorMessage();
		buffer += '\n';
	}

	if (FAILED(result = CoCreateInstance(__uuidof(AppVisibility), NULL, CLSCTX_INPROC_SERVER, IID_IAppVisibility, reinterpret_cast<LPVOID *>(&run.app_visibility))))
	{
		buffer += L"Initialization of IAV failed. Exception from HRESULT: ";
		buffer += _com_error(result).ErrorMessage();
		buffer += '\n';
	}

	OutputDebugString(buffer.c_str());
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

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int)
{
	// If there already is another instance running, tell it to exit
	if (!IsSingleInstance()) {
		HWND oldInstance = FindWindow(App::NAME, L"TrayWindow");
		SendMessage(oldInstance, Tray::NEW_TTB_INSTANCE, NULL, NULL);
	}

	// Initialize COM, UWP, set DPI awareness, and acquire a VirtualDesktopManager and AppVisibility interface
	InitializeAPIs();

	// Get configuration file paths
	HRESULT error = GetPaths();
	if (FAILED(error))
	{
		std::wstring message;
		message += L"Failed to determine configuration files locations!\n\nProgram will exit.\n\nException from HRESULT: ";
		message += _com_error(error).ErrorMessage();

		MessageBox(NULL, message.c_str(), (std::wstring(App::NAME) + L" - Fatal error").c_str(), MB_ICONERROR | MB_OK);
		return 1;
	}

	// If the configuration files don't exist, restore the files and show welcome to the users
	if (!CheckAndRunWelcome()) {
		return 0;
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

	// Scan windows to start taskbar with the correct mode immediatly
	if (opt.dynamicws || opt.peek == Taskbar::Dynamic)
	{
		EnumWindows(&EnumWindowsProcess, NULL);
	}

	// Undoc'd, allows to detect when Aero Peek starts and stops
	HWINEVENTHOOK hook = SetWinEventHook(0x21, 0x22, NULL, HandleAeroPeekEvent, 0, 0, WINEVENT_OUTOFCONTEXT);

	// Message loop
	while (run.run) {
		MSG msg;
		if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		SetTaskbarBlur();
		Sleep(10);
	}

	UnhookWinEvent(hook);

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
		opt.peek = Taskbar::Enabled;
		opt.dynamicstart = false;
		opt.dynamicws = false;
		SetTaskbarBlur();
	}

	// Close the uniqueness handle to allow other instances to run
	CloseHandle(run.app_handle);

	// Uninitialize UWP and COM
	CoUninitialize();
	Windows::Foundation::Uninitialize();

	// Notify Explorer we are exiting
	Shell_NotifyIcon(NIM_DELETE, &run.tray);

	// Exit
	return 0;
}

#pragma endregion