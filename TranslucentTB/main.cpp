#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <tchar.h>
#include <map>
#include <psapi.h>
#include <ShlObj.h>
#include <Shlwapi.h>
#include <roapi.h>
#include <algorithm>
#include <ShellScalingAPI.h>
#include <shellapi.h>
#include "resource.h"

using namespace Windows::Foundation;

#pragma region Enumerations

enum TASKBARSTATE {		// enum to store states of a taskbar
	Normal,				// Proceed as normal. If no dynamic options are set, act as it says in opt.taskbar_appearance
	WindowMaximised,	// There is a window which is maximised on the monitor this HWND is in. Display as blurred.
	StartMenuOpen		// The Start Menu is open on the monitor this HWND is in. Display as it would be without TranslucentTB active.
};

enum EXITREASON {		// enum to store possible exit reasons
	NewInstance,		// New instance told us to exit
	UserAction			// Triggered by the user
};

enum PEEKSTATE {	// enum to store the user's Aero Peek settings
	Disabled,		// Hide the button
	Dynamic,		// Show when a window is maximised
	Enabled			// Show the button
};

enum ACCENTSTATE {								// Values passed to SetWindowCompositionAttribute determining the appearance of a window
	ACCENT_ENABLE_GRADIENT = 1,					// Use a solid color specified by nColor. This mode doesn't care about the alpha channel.
	ACCENT_ENABLE_TRANSPARENTGRADIENT = 2,		// Use a tinted transparent overlay. nColor is the tint color, sending nothing results in it interpreted as 0x00000000 (totally transparent, blends in with desktop)
	ACCENT_ENABLE_BLURBEHIND = 3,				// Use a tinted blurry overlay. nColor is the tint color, sending nothing results in it interpreted as 0x00000000 (totally transparent, blends in with desktop)
	ACCENT_ENABLE_FLUENT = 4,					// Use fluent design-like aspect. nColor is tint color.

	ACCENT_FOLLOW_OPT = 149,					// (Fake value) Respect what defined in opt.taskbar_appearance
	ACCENT_ENABLE_TINTED = 150,					// (Fake value) Dynamic windows tinted
	ACCENT_NORMAL = 151							// (Fake value) Emulate regular taskbar appearance
};

enum WindowCompositionAttribute {				// Possible kinds of data sent to SetWindowCompositionAttribute
	// ...
	WCA_ACCENT_POLICY = 19						// The data sent is an ACCENTPOLICY struct
	// ...
};

#pragma endregion

#pragma region Structures

struct ACCENTPOLICY					// Determines how a window's transparent region will be painted
{
	ACCENTSTATE nAccentState;		// Appearance
	int nFlags;						// Nobody knows how this value works
	UINT nColor;					// A color in the format AABBGGRR
	int nAnimationId;				// Nobody knows how this value works
};

struct WINCOMPATTRDATA							// Composition Attributes
{
	WindowCompositionAttribute nAttribute;		// Type of the data passed in nAttribute
	PVOID pData;								// Some data
	ULONG ulDataSize;							// Size of the data passed in nAttribute
};

struct TASKBARPROPERTIES	// Relevant info on a taskbar
{
	HMONITOR hmon;			// Monitor it lives on
	TASKBARSTATE state;		// How we should handle it
};

static struct OPTIONS											// User settings
{
	ACCENTSTATE taskbar_appearance = ACCENT_ENABLE_BLURBEHIND;	// Appearance of the taskbar
	UINT color = 0x00000000;									// Color to apply to the taskbar
	bool dynamicws = false;										// Wether dynamic windows are enabled
	ACCENTSTATE dynamic_ws_state = ACCENT_ENABLE_BLURBEHIND;	// State to activate when a window is maximised
	bool dynamicstart = false;									// Wether dynamic start is enabled
	PEEKSTATE peek = Enabled;									// Controls how the Aero Peek button is handled
	std::vector<std::wstring> blacklisted_classes;				// List of window classes the user blacklisted
	std::vector<std::wstring> blacklisted_filenames;			// List of executable filenames the user blacklisted
	std::vector<std::wstring> blacklisted_titles;				// List of window titles the user blacklisted
} opt;

static struct RUNTIME															// Used to store things relevant only to runtime
{
	EXITREASON exit_reason = UserAction;										// Determines if current configuration should be saved when we exit
	IVirtualDesktopManager *desktop_manager = NULL;								// Used to detect if a window is in the current virtual desktop. Don't forget to check for null on this one
	UINT WM_TASKBARCREATED;														// Message received when Explorer restarts
	UINT NEW_TTB_INSTANCE;														// Message sent when an instance should exit because a new one started
	HWND main_taskbar;															// Handle to taskbar that shows on main monitor
	std::map<HWND, TASKBARPROPERTIES> taskbars;									// Map for all taskbars and their properties
	bool should_show_peek;														// Set by the EnumWindowsProcess and determines if peek should be shown when in dynamic mode
	bool run = true;															// Set to false to break out of the main program loop
	HMENU popup;																// Tray icon popup
	int opacity;																// Opacity override. Later stored in opt.color
	HANDLE ev;																	// Handle to an event. Used for uniqueness
	HMENU menu;																	// Tray icon context menu
	NOTIFYICONDATA tray;														// Tray icon
	HWND tray_hwnd;																// Tray window handle
	bool fluent_available;														// Wether ACCENT_ENABLE_FLUENT works
} run;

const static struct CONSTANTS											// Constants. What else do you need?
{
	const LPCWSTR guid = L"344635E9-9AE4-4E60-B128-D53E25AB70A7";		// Used to prevent two instances running at the same time
	const int WM_NOTIFY_TB = 3141;										// Message id for tray callback
	const LPCWSTR program_name = L"TranslucentTB";						// Sounds weird, but prevents typos
} cnst;

#pragma endregion

#pragma region That one function that does all the magic

typedef BOOL(WINAPI*pSetWindowCompositionAttribute)(HWND, WINCOMPATTRDATA*);
static pSetWindowCompositionAttribute SetWindowCompositionAttribute = (pSetWindowCompositionAttribute)GetProcAddress(GetModuleHandle(TEXT("user32.dll")), "SetWindowCompositionAttribute");

void SetWindowBlur(HWND hWnd, ACCENTSTATE appearance = ACCENT_FOLLOW_OPT)
{
	if (SetWindowCompositionAttribute)
	{
		ACCENTPOLICY policy;

		if (appearance != ACCENT_FOLLOW_OPT) // Custom taskbar appearance is set
		{
			if (opt.dynamic_ws_state == ACCENT_ENABLE_TINTED)
			{ // dynamic-ws is set to tint
				if (appearance == ACCENT_ENABLE_TINTED) { policy = { ACCENT_ENABLE_TRANSPARENTGRADIENT, 2, opt.color, 0 }; } // Window is maximised
				else { policy = { ACCENT_ENABLE_TRANSPARENTGRADIENT, 2, 0x00000000, 0 }; } // Desktop is shown (this shouldn't ever be called tho, just in case)
			}
			else { policy = { appearance, 2, opt.color, 0 }; }
		}
		else { // Use the defaults
			if (opt.dynamic_ws_state == ACCENT_ENABLE_TINTED) { policy = { ACCENT_ENABLE_TRANSPARENTGRADIENT, 2, 0x00000000, 0 }; } // dynamic-ws is tint and desktop is shown
			else if (opt.taskbar_appearance == ACCENT_NORMAL) { policy = { ACCENT_ENABLE_TRANSPARENTGRADIENT, 2, 0xd9000000, 0 }; } // normal gradient color
			else { policy = { opt.taskbar_appearance, 2, opt.color, 0 }; }
		}

		WINCOMPATTRDATA data = { WCA_ACCENT_POLICY, &policy, sizeof(ACCENTPOLICY) };
		SetWindowCompositionAttribute(hWnd, &data);
	}
}

#pragma endregion

#pragma region IO help

bool FileExists(std::wstring path)
{
	std::ifstream infile(path);
	return infile.good();
}

#pragma endregion

#pragma region Configuration

void AddToStartup()
{
	HMODULE hModule = GetModuleHandle(NULL);
	TCHAR path[MAX_PATH];
	GetModuleFileName(hModule, path, MAX_PATH);
	std::wstring unsafePath = path;
	std::wstring progPath = L"\"" + unsafePath + L"\"";
	HKEY hkey = NULL;
	LONG createStatus = RegCreateKey(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", &hkey); //Creates a key
	LONG status = RegSetValueEx(hkey, cnst.program_name, 0, REG_SZ, (BYTE *)progPath.c_str(), (DWORD)((progPath.size() + 1) * sizeof(wchar_t)));
}

void ParseSingleConfigOption(std::wstring arg, std::wstring value)
{
	if (arg == L"accent")
	{
		if (value == L"blur")
			opt.taskbar_appearance = ACCENT_ENABLE_BLURBEHIND;
		else if (value == L"opaque")
			opt.taskbar_appearance = ACCENT_ENABLE_GRADIENT;
		else if (value == L"transparent" ||
			value == L"translucent" ||
			value == L"clear")
			opt.taskbar_appearance = ACCENT_ENABLE_TRANSPARENTGRADIENT;
		else if (value == L"normal")
			opt.taskbar_appearance = ACCENT_NORMAL;
		else if (value == L"fluent" && run.fluent_available)
			opt.taskbar_appearance = ACCENT_ENABLE_FLUENT;
	}
	else if (arg == L"dynamic-ws")
	{
		if (value == L"true" ||
			value == L"enable")
		{
			opt.dynamicws = true;
		}
		else if (value == L"tint")
		{
			opt.dynamicws = true;
			opt.dynamic_ws_state = ACCENT_ENABLE_TINTED;
		}
		else if (value == L"blur")
		{
			opt.dynamicws = true;
			opt.dynamic_ws_state = ACCENT_ENABLE_BLURBEHIND;
		}
		else if (value == L"opaque")
		{
			opt.dynamicws = true;
			opt.dynamic_ws_state = ACCENT_ENABLE_GRADIENT;
		}
		else if (value == L"normal")
		{
			opt.dynamicws = true;
			opt.dynamic_ws_state = ACCENT_NORMAL;
		}
		else if (value == L"fluent" && run.fluent_available)
		{
			opt.dynamicws = true;
			opt.dynamic_ws_state = ACCENT_ENABLE_FLUENT;
		}
	}
	else if (arg == L"dynamic-start")
	{
		if (value == L"true" ||
			value == L"enable")
		{
			opt.dynamicstart = true;
		}
	}
	else if (arg == L"color" ||
		arg == L"tint")
	{
		if (value.find(L'#') == 0)
			value = value.substr(1, value.length() - 1);

		size_t numchars = 0;
		// heh make sure we don't run into overflow errors
		// TODO use proper range here and check for overflows. Write to logfile and warn user of error.
		unsigned long long parsed = std::stoull(value, &numchars, 16);

		opt.color =
			(parsed & 0xFF000000) +
			((parsed & 0x00FF0000) >> 16) +
			(parsed & 0x0000FF00) +
			((parsed & 0x000000FF) << 16);
	}
	else if (arg == L"opacity")
	{
		// TODO Same here. Should check range and warn user if the value doesn't fit.
		size_t numchars = 0;
		int parsed = std::stoi(value, &numchars, 10);

		if (parsed < 0)
			parsed = 0;
		else if (parsed > 255)
			parsed = 255;

		run.opacity = parsed;
	}
	else if (arg == L"peek")
	{
		if (value == L"hide")
		{
			opt.peek = Disabled;
		}
		else if (value == L"dynamic")
		{
			opt.peek = Dynamic;
		}
	}
}

void ParseConfigFile(std::wstring path)
{
	std::wifstream configstream(path);

	for (std::wstring line; std::getline(configstream, line); )
	{
		// Skip comments
		size_t comment_index = line.find(L';');
		if (comment_index == 0)
			continue;
		else
			line = line.substr(0, comment_index);

		size_t split_index = line.find(L'=');
		std::wstring key = line.substr(0, split_index);
		std::wstring val = line.substr(split_index + 1, line.length() - split_index - 1);

		ParseSingleConfigOption(key, val);
	}

	if (run.opacity >= 0)
	{
		opt.color = (run.opacity << 24) +
			(opt.color & 0x00FFFFFF);
	}
}

void SaveConfigFile(std::wstring configfile)
{
	if (!configfile.empty())
	{
		using namespace std;
		wofstream configstream(configfile);

		configstream << L"; ===========================================================================" << endl;
		configstream << L"; Warning" << endl;
		configstream << L"; File gets overwritten on exit. Only edit when " << cnst.program_name << L" is not running." << endl;
		configstream << L"; ===========================================================================" << endl;
		configstream << endl;

		configstream << L"; Taskbar appearance: opaque, clear, normal, or blur (default)." << endl;

		configstream << L"accent=";

		if (opt.taskbar_appearance == ACCENT_ENABLE_GRADIENT)
			configstream << L"opaque" << endl;
		else if (opt.taskbar_appearance == ACCENT_ENABLE_TRANSPARENTGRADIENT)
			configstream << L"clear" << endl;
		else if (opt.taskbar_appearance == ACCENT_ENABLE_BLURBEHIND)
			configstream << L"blur" << endl;
		else if (opt.taskbar_appearance == ACCENT_NORMAL)
			configstream << L"normal" << endl;
		else if (opt.taskbar_appearance == ACCENT_ENABLE_FLUENT)
			configstream << L"fluent" << endl;

		configstream << endl;
		configstream << L"; Dynamic states: Window States and (WIP) Start Menu" << endl;
		configstream << L"; dynamic windows: opaque, tint, normal, or blur (default)." << endl;
		configstream << L"; dynamic windows can be used in conjunction with a custom color and non-zero opacity!" << endl;
		configstream << L"; you can also set an accent value, which will represent the state of dynamic windows when there is no window maximised" << endl;

		if (!opt.dynamicws)
			configstream << L"; ";

		configstream << L"dynamic-ws=";

		if (opt.dynamic_ws_state == ACCENT_ENABLE_BLURBEHIND)
			configstream << L"blur" << endl;
		else if (opt.dynamic_ws_state == ACCENT_ENABLE_TINTED)
			configstream << L"tint" << endl;
		else if (opt.dynamic_ws_state == ACCENT_ENABLE_GRADIENT)
			configstream << L"opaque" << endl;
		else if (opt.dynamic_ws_state == ACCENT_NORMAL)
			configstream << L"normal" << endl;
		else if (opt.dynamic_ws_state == ACCENT_ENABLE_FLUENT)
			configstream << L"fluent" << endl;
		else
			configstream << L"enable" << endl;

		if (!opt.dynamicstart)
			configstream << L"; ";

		configstream << L"dynamic-start=enable" << endl;

		configstream << endl;
		configstream << L"; Color and opacity of the taskbar." << endl;

		unsigned int bitreversed =
			(opt.color & 0xFF000000) +
			((opt.color & 0x00FF0000) >> 16) +
			(opt.color & 0x0000FF00) +
			((opt.color & 0x000000FF) << 16);
		configstream << L"color=" << hex << bitreversed << L"    ; A color in hexadecimal notation." << endl;
		configstream << L"opacity=" << to_wstring((opt.color & 0xFF000000) >> 24) << L"    ; A value in the range 0 to 255." << endl;
		configstream << endl;
		configstream << L"; Controls how the Aero Peek button behaves" << endl;
		configstream << L"peek=";
		if (opt.peek == Disabled)
			configstream << L"hide" << endl;
		else if (opt.peek == Dynamic)
			configstream << L"dynamic" << endl;
		else
			configstream << L"show" << endl;
	}
}

std::wstring Trim(std::wstring& str)
{
	size_t first = str.find_first_not_of(' ');
	size_t last = str.find_last_not_of(' ');

	if (first == std::wstring::npos)
	{
		return std::wstring(L"");
	}
	return str.substr(first, (last - first + 1));
}

std::vector<std::wstring> ParseByDelimiter(std::wstring row, std::wstring delimiter = L",")
{
	std::vector<std::wstring> result;
	std::wstring token;
	size_t pos = 0;
	while ((pos = row.find(delimiter)) != std::string::npos)
	{
		token = Trim(row.substr(0, pos));
		result.push_back(token);
		row.erase(0, pos + delimiter.length());
	}
	return result;
}

void ParseBlacklistFile(std::wstring filename)
{
	std::wifstream excludesfilestream(filename);

	std::wstring delimiter = L","; // Change to change the char(s) used to split,

	for (std::wstring line; std::getline(excludesfilestream, line); )
	{
		size_t comment_index = line.find(L';');
		if (comment_index == 0)
			continue;
		else
			line = line.substr(0, comment_index);

		if (line.length() > delimiter.length())
		{
			if (line.compare(line.length() - delimiter.length(), delimiter.length(), delimiter))
			{
				line.append(delimiter);
			}
		}
		std::wstring line_lowercase = line;
		std::transform(line_lowercase.begin(), line_lowercase.end(), line_lowercase.begin(), tolower);
		if (line_lowercase.substr(0, 5) == L"class")
		{
			opt.blacklisted_classes = ParseByDelimiter(line, delimiter);
			opt.blacklisted_classes.erase(opt.blacklisted_classes.begin());
		}
		else if (line_lowercase.substr(0, 5) == L"title" ||
			line.substr(0, 13) == L"windowtitle")
		{
			opt.blacklisted_titles = ParseByDelimiter(line, delimiter);
			opt.blacklisted_titles.erase(opt.blacklisted_titles.begin());
		}
		else if (line_lowercase.substr(0, 7) == L"exename")
		{
			opt.blacklisted_filenames = ParseByDelimiter(line, delimiter);
			opt.blacklisted_filenames.erase(opt.blacklisted_filenames.begin());
		}
	}
}

#pragma endregion

#pragma region Utilities

void RefreshHandles()
{
	HWND secondtaskbar = NULL;
	TASKBARPROPERTIES _properties;

	run.taskbars.clear();
	run.main_taskbar = FindWindowW(L"Shell_TrayWnd", NULL);

	_properties.hmon = MonitorFromWindow(run.main_taskbar, MONITOR_DEFAULTTOPRIMARY);
	_properties.state = Normal;
	run.taskbars.insert(std::make_pair(run.main_taskbar, _properties));
	while (secondtaskbar = FindWindowEx(0, secondtaskbar, L"Shell_SecondaryTrayWnd", NULL))
	{
		_properties.hmon = MonitorFromWindow(secondtaskbar, MONITOR_DEFAULTTOPRIMARY);
		_properties.state = Normal;
		run.taskbars.insert(std::make_pair(secondtaskbar, _properties));
	}
}

void TogglePeek(bool status)
{
	static bool cached_peek = true;

	if (status != cached_peek)
	{
		HWND _tray = FindWindowEx(run.main_taskbar, NULL, L"TrayNotifyWnd", NULL);
		HWND _peek = FindWindowEx(_tray, NULL, L"TrayShowDesktopButtonWClass", NULL);
		HWND _overflow = FindWindowEx(_tray, NULL, L"Button", NULL);

		ShowWindow(_peek, status ? SW_SHOWNORMAL : SW_HIDE);

		// This is a really terrible hack, but it's the only way I found to make the changes reflect instantly.
		// Toggles the overflow area popup twice. Nearly imperceptible.
		SendMessage(_overflow, WM_LBUTTONUP, NULL, NULL);
		SendMessage(_overflow, WM_LBUTTONUP, NULL, NULL);

		cached_peek = status;
	}
}

bool IsWindowBlacklisted(HWND hWnd)
{
	// Get respective attributes
	TCHAR className[MAX_PATH];
	TCHAR exeName_path[MAX_PATH];
	TCHAR windowTitle[MAX_PATH];
	GetClassName(hWnd, className, _countof(className));
	GetWindowText(hWnd, windowTitle, _countof(windowTitle));

	DWORD ProcessId;
	GetWindowThreadProcessId(hWnd, &ProcessId);
	HANDLE processhandle = OpenProcess(PROCESS_QUERY_INFORMATION, false, ProcessId);
	GetModuleFileNameEx(processhandle, NULL, exeName_path, _countof(exeName_path));

	std::wstring exeName = PathFindFileNameW(exeName_path);
	std::wstring w_WindowTitle = windowTitle;

	// Check if the different vars are in their respective vectors
	for (auto & value : opt.blacklisted_classes)
	{
		if (className == value.c_str()) { return true; }
	}
	for (auto & value : opt.blacklisted_filenames)
	{
		if (exeName == value) { return true; }
	}
	for (auto & value : opt.blacklisted_titles)
	{
		if (w_WindowTitle.find(value) != std::wstring::npos)
		{
			return true;
		}
	}
	return false;
}

#pragma endregion

#pragma region Tray

bool CheckPopupItem(UINT item_to_check, bool state)
{
	return CheckMenuItem(run.popup, item_to_check, MF_BYCOMMAND | (state ? MF_CHECKED : MF_UNCHECKED) | MF_ENABLED);
}

void RefreshMenu()
{
	UINT radio_to_check_regular = NULL;
	UINT radio_to_check_dynamic = NULL;
	UINT radio_to_check_peek    = NULL;

	if (opt.taskbar_appearance == ACCENT_ENABLE_BLURBEHIND)
	{
		radio_to_check_regular = IDM_BLUR;
	}
	else if (opt.taskbar_appearance == ACCENT_ENABLE_TRANSPARENTGRADIENT)
	{
		radio_to_check_regular = IDM_CLEAR;
	}
	else if (opt.taskbar_appearance == ACCENT_NORMAL)
	{
		radio_to_check_regular = IDM_NORMAL;
	}
	else if (opt.taskbar_appearance == ACCENT_ENABLE_GRADIENT)
	{
		radio_to_check_regular = IDM_OPAQUE;
	}
	else if (opt.taskbar_appearance == ACCENT_ENABLE_FLUENT)
	{
		radio_to_check_regular = IDM_FLUENT;
	}
	else
	{
		OutputDebugString(L"Unable to determine which radio item to check for regular state!");
	}

	if (opt.dynamic_ws_state == ACCENT_ENABLE_BLURBEHIND)
	{
		radio_to_check_dynamic = IDM_DYNAMICWS_BLUR;
	}
	else if (opt.dynamic_ws_state == ACCENT_ENABLE_TINTED)
	{
		radio_to_check_dynamic = IDM_DYNAMICWS_CLEAR;
	}
	else if (opt.dynamic_ws_state == ACCENT_NORMAL)
	{
		radio_to_check_dynamic = IDM_DYNAMICWS_NORMAL;
	}
	else if (opt.dynamic_ws_state == ACCENT_ENABLE_GRADIENT)
	{
		radio_to_check_dynamic = IDM_DYNAMICWS_OPAQUE;
	}
	else if (opt.dynamic_ws_state == ACCENT_ENABLE_FLUENT)
	{
		radio_to_check_dynamic = IDM_DYNAMICWS_FLUENT;
	}
	else
	{
		OutputDebugString(L"Unable to determine which radio item to check for dynamic state!");
	}



	if (opt.peek == Enabled)
	{
		radio_to_check_peek = IDM_PEEK;
	}
	else if (opt.peek == Dynamic)
	{
		radio_to_check_peek = IDM_DPEEK;
	}
	else if (opt.peek == Disabled)
	{
		radio_to_check_peek = IDM_NOPEEK;
	}
	else
	{
		OutputDebugString(L"Unable to determine which radio item to check for peek!");
	}

	CheckMenuRadioItem(run.popup, IDM_BLUR, IDM_OPAQUE, radio_to_check_regular, MF_BYCOMMAND);
	CheckMenuRadioItem(run.popup, IDM_DYNAMICWS_BLUR, IDM_DYNAMICWS_OPAQUE, radio_to_check_dynamic, MF_BYCOMMAND);
	CheckMenuRadioItem(run.popup, IDM_PEEK, IDM_NOPEEK, radio_to_check_peek, MF_BYCOMMAND);

	for (INT item : { IDM_FLUENT, IDM_DYNAMICWS_FLUENT})
		EnableMenuItem(run.popup, item, MF_BYCOMMAND | (run.fluent_available ? MF_ENABLED : MF_GRAYED));

	for (INT item : { IDM_DYNAMICWS_BLUR, IDM_DYNAMICWS_CLEAR, IDM_DYNAMICWS_NORMAL, IDM_DYNAMICWS_OPAQUE, IDM_DYNAMICWS_FLUENT })
		EnableMenuItem(run.popup, item, MF_BYCOMMAND | (opt.dynamicws ? MF_ENABLED : MF_GRAYED));

	CheckPopupItem(IDM_DYNAMICWS, opt.dynamicws);
	CheckPopupItem(IDM_DYNAMICSTART, opt.dynamicstart);
	CheckPopupItem(IDM_AUTOSTART, RegGetValue(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", cnst.program_name, RRF_RT_REG_SZ, NULL, NULL, NULL) == ERROR_SUCCESS);
}

void InitializeTray()
{
	run.tray.cbSize = sizeof(run.tray);
	run.tray.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(MAINICON));
	run.tray.hWnd = run.tray_hwnd;
	wcscpy_s(run.tray.szTip, cnst.program_name);
	run.tray.uCallbackMessage = cnst.WM_NOTIFY_TB;
	run.tray.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
	run.tray.uID = 101;
	Shell_NotifyIcon(NIM_ADD, &run.tray);
	Shell_NotifyIcon(NIM_SETVERSION, &run.tray);
	RefreshMenu();
}

LRESULT CALLBACK TrayCallback(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_CLOSE)
	{
		PostQuitMessage(0);
	}
	else if (message == cnst.WM_NOTIFY_TB)
	{
		if (lParam == WM_LBUTTONUP || lParam == WM_RBUTTONUP)
		{
			POINT pt;
			GetCursorPos(&pt);
			SetForegroundWindow(hWnd);
			UINT tray = TrackPopupMenu(run.menu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, pt.x, pt.y, 0, hWnd, NULL);
			switch (tray) // TODO: Add menu items for colors, and one to open config file locations, and one to reset ApplicationFrameHost (after reset if there is still a bad window, show details to help in bug report)
			{
			case IDM_BLUR:
				opt.taskbar_appearance = ACCENT_ENABLE_BLURBEHIND;
				break;
			case IDM_CLEAR:
				opt.taskbar_appearance = ACCENT_ENABLE_TRANSPARENTGRADIENT;
				break;
			case IDM_NORMAL:
				opt.taskbar_appearance = ACCENT_NORMAL;
				break;
			case IDM_OPAQUE:
				opt.taskbar_appearance = ACCENT_ENABLE_GRADIENT;
				break;
			case IDM_FLUENT:
				opt.taskbar_appearance = ACCENT_ENABLE_FLUENT;
				break;
			case IDM_DYNAMICWS:
				opt.dynamicws = !opt.dynamicws;
				break;
			case IDM_DYNAMICWS_BLUR:
				opt.dynamic_ws_state = ACCENT_ENABLE_BLURBEHIND;
				break;
			case IDM_DYNAMICWS_CLEAR:
				opt.dynamic_ws_state = ACCENT_ENABLE_TINTED;
				break;
			case IDM_DYNAMICWS_NORMAL:
				opt.dynamic_ws_state = ACCENT_NORMAL;
				break;
			case IDM_DYNAMICWS_OPAQUE:
				opt.dynamic_ws_state = ACCENT_ENABLE_GRADIENT;
				break;
			case IDM_DYNAMICWS_FLUENT:
				opt.dynamic_ws_state = ACCENT_ENABLE_FLUENT;
				break;
			case IDM_DYNAMICSTART:
				opt.dynamicstart = !opt.dynamicstart;
				break;
			case IDM_PEEK:
				opt.peek = Enabled;
				break;
			case IDM_DPEEK:
				opt.peek = Dynamic;
				break;
			case IDM_NOPEEK:
				opt.peek = Disabled;
				break;
			case IDM_AUTOSTART: // TODO: Use UWP Apis
				if (RegGetValue(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", cnst.program_name, RRF_RT_REG_SZ, NULL, NULL, NULL) == ERROR_SUCCESS)
				{
					HKEY hkey = NULL;
					RegCreateKey(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", &hkey);
					RegDeleteValue(hkey, cnst.program_name);
				}
				else {
					AddToStartup();
				}
				break;
			case IDM_EXIT:
				run.run = false;
				break;
			}
			RefreshMenu();
		}
	}
	else if (message == run.WM_TASKBARCREATED)
	{
		RefreshHandles();
		InitializeTray();
	}
	else if (message == run.NEW_TTB_INSTANCE) {
		run.exit_reason = NewInstance;
		run.run = false;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

#pragma endregion

#pragma region Main logic

BOOL CALLBACK EnumWindowsProcess(HWND hWnd, LPARAM lParam)
{
	WINDOWPLACEMENT result = {};
	::GetWindowPlacement(hWnd, &result);
	if (result.showCmd == SW_MAXIMIZE) {
		BOOL on_current_desktop = true;
		if (run.desktop_manager)
			run.desktop_manager->IsWindowOnCurrentVirtualDesktop(hWnd, &on_current_desktop);
		if (IsWindowVisible(hWnd) && on_current_desktop)
		{
			if (!IsWindowBlacklisted(hWnd))
			{
				HMONITOR _monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTOPRIMARY);
				for (auto &taskbar : run.taskbars)
				{

					if (taskbar.second.hmon == _monitor)
					{
						if (taskbar.second.state != StartMenuOpen)
							taskbar.second.state = WindowMaximised;
						if (opt.peek == Dynamic && taskbar.first == run.main_taskbar)
							run.should_show_peek = true;
					}
				}
			}
		}
	}
	return true;
}

void SetTaskbarBlur()
{
	static int counter = 0;

	if (counter >= 10)	// Change this if you want to change the time it takes for the program to update
	{					// 100 = 1 second; we use 10, because the difference is less noticeable and it has
						// no large impact on CPU. We can change this if we feel that CPU is more important
						// than response time.
		run.should_show_peek = (opt.peek == Enabled);

		for (auto &taskbar : run.taskbars)
		{
			taskbar.second.state = Normal; // Reset taskbar state
		}
		if (opt.dynamicws || opt.peek == Dynamic) {
			counter = 0;
			EnumWindows(&EnumWindowsProcess, NULL);
		}

		if (opt.dynamicstart)
		{
			HWND foreground;
			TCHAR ForehWndClass[MAX_PATH];
			TCHAR ForehWndName[MAX_PATH];

			foreground = GetForegroundWindow();
			GetWindowText(foreground, ForehWndName, _countof(ForehWndName));
			GetClassName(foreground, ForehWndClass, _countof(ForehWndClass));

			if (!_tcscmp(ForehWndClass, _T("Windows.UI.Core.CoreWindow")) &&
				(!_tcscmp(ForehWndName, _T("Search")) || !_tcscmp(ForehWndName, _T("Cortana"))))
			{
				// Detect monitor Start Menu is open on
				HMONITOR _monitor;
				_monitor = MonitorFromWindow(foreground, MONITOR_DEFAULTTOPRIMARY);
				for (auto &taskbar : run.taskbars)
				{
					if (taskbar.second.hmon == _monitor)
					{
						taskbar.second.state = StartMenuOpen;
					}
					else {
						taskbar.second.state = Normal;
					}
				}
			}
		}
	}

	for (auto const &taskbar : run.taskbars)
	{
		if (taskbar.second.state == WindowMaximised) {
			SetWindowBlur(taskbar.first, opt.dynamic_ws_state);
			// A window is maximised; let's make sure that we blur the window.
		}
		else if (taskbar.second.state == Normal) {
			SetWindowBlur(taskbar.first);  // Taskbar should be normal, call using normal transparency settings
		}
	}
	TogglePeek(run.should_show_peek);
	counter++;
}

#pragma endregion

#pragma region Startup

bool SingleInstance()
{
	run.ev = CreateEvent(NULL, TRUE, FALSE, cnst.guid);
	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		return false;
	}
	return true;
}

bool IsFluentPresent()
{
	typedef NTSTATUS(__stdcall *pRtlGetVersion)(PRTL_OSVERSIONINFOW);
	HMODULE ntdll = GetModuleHandle(L"ntdll");
	pRtlGetVersion RtlGetVersion = (pRtlGetVersion)GetProcAddress(ntdll, "RtlGetVersion");
	RTL_OSVERSIONINFOW versionInfo;
	RtlGetVersion(&versionInfo);

	return versionInfo.dwBuildNumber >= 17063;
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPreInst, _In_ LPSTR pCmdLine, _In_ int nCmdShow)
{
	if (FAILED(SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE))) { OutputDebugStringW(L"Per-monitor DPI scaling failed\n"); }

	LPWSTR localAppData;

	if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_DEFAULT, NULL, &localAppData)))
	{
		MessageBox(NULL, L"Failed to get LocalAppData folder location!\n\nProgram will exit.", (std::wstring(cnst.program_name) + L" - Fatal error").c_str(), MB_ICONERROR | MB_OK);
		return 1;
	}

	TCHAR configFolder[MAX_PATH];
	TCHAR excludeFile[MAX_PATH];
	TCHAR configFile[MAX_PATH];
	TCHAR exeFolder[MAX_PATH];
	TCHAR stockConfigFile[MAX_PATH];
	TCHAR stockExcludeFile[MAX_PATH];

	PathCombine(configFolder, localAppData, cnst.program_name);
	PathCombine(configFile, configFolder, L"config.cfg");
	PathCombine(excludeFile, configFolder, L"dynamic-ws-exclude.csv");

	HMODULE hModule = GetModuleHandle(NULL);
	GetModuleFileName(hModule, exeFolder, MAX_PATH);
	PathRemoveFileSpec(exeFolder);

	PathCombine(stockConfigFile, exeFolder, L"config.cfg");
	PathCombine(stockExcludeFile, exeFolder, L"dynamic-ws-exclude.csv");

	if (!PathFileExists(configFolder))
	{
		// TODO: First run experience
		CreateDirectory(configFolder, NULL);
	}

	if (!PathFileExists(configFile))
	{
		CopyFile(stockConfigFile, configFile, FALSE);
	}
	if (!PathFileExists(excludeFile))
	{
		CopyFile(stockExcludeFile, excludeFile, FALSE);
	}

	run.fluent_available = IsFluentPresent();

	ParseConfigFile(configFile); // Config file settings
	ParseBlacklistFile(excludeFile);

	run.NEW_TTB_INSTANCE = RegisterWindowMessage(L"NewTTBInstance");
	if (!SingleInstance()) {
		HWND oldInstance = FindWindow(cnst.program_name, L"TrayWindow");
		SendMessage(oldInstance, run.NEW_TTB_INSTANCE, NULL, NULL);
	}

	MSG msg; // for message translation and dispatch
	run.popup = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_POPUP_MENU));
	run.menu = GetSubMenu(run.popup, 0);
	WNDCLASSEX wnd = { 0 };

	wnd.hInstance = hInstance;
	wnd.lpszClassName = cnst.program_name;
	wnd.lpfnWndProc = TrayCallback;
	wnd.style = CS_HREDRAW | CS_VREDRAW;
	wnd.cbSize = sizeof(WNDCLASSEX);

	wnd.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wnd.hCursor = LoadCursor(NULL, IDC_ARROW);
	wnd.hbrBackground = (HBRUSH)BLACK_BRUSH;
	RegisterClassEx(&wnd);

	run.tray_hwnd = CreateWindowEx(WS_EX_TOOLWINDOW, cnst.program_name, L"TrayWindow", WS_OVERLAPPEDWINDOW, 0, 0,
		400, 400, NULL, NULL, hInstance, NULL);

	InitializeTray();

	ShowWindow(run.tray_hwnd, WM_SHOWWINDOW);

	// Store stuff
	if (FAILED(Initialize())) { OutputDebugStringW(L"Initialization of UWP APIs failed. Unable to manipulate startup entry."); }
	//Windows::ApplicationModel::StartupTask::GetForCurrentPackageAsync()->GetResults()->GetAt(1)->State;
	// This should not make it crash once it gets packaged into an APPX.

	//Virtual Desktop stuff
	if (FAILED(::CoInitialize(NULL))) { OutputDebugStringW(L"Initialization of COM failed, VirtualDesktopManager will probably fail too.\n"); }
	HRESULT desktop_success = ::CoCreateInstance(__uuidof(VirtualDesktopManager), NULL, CLSCTX_INPROC_SERVER, IID_IVirtualDesktopManager, (void **)&run.desktop_manager);
	if (FAILED(desktop_success)) { OutputDebugStringW(L"Initialization of VirtualDesktopManager failed, dynamic windows will not support Windows virtual desktops.\n"); }

	RefreshHandles();
	if (opt.dynamicws || opt.peek == Dynamic)
	{
		EnumWindows(&EnumWindowsProcess, NULL); // Putting this here so there isn't a
												// delay between when you start the
												// program and when the taskbar goes blurry
	}
	run.WM_TASKBARCREATED = RegisterWindowMessage(L"TaskbarCreated");

	while (run.run) {
		if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		SetTaskbarBlur();
		Sleep(10);
	}

	if (run.exit_reason != NewInstance)
	{
		SaveConfigFile(configFile);

		// Restore default taskbar appearance
		opt.taskbar_appearance = run.fluent_available ? ACCENT_ENABLE_FLUENT : ACCENT_ENABLE_TRANSPARENTGRADIENT;
		opt.color = 0x99000000;
		opt.peek = Enabled;
		opt.dynamicstart = false;
		opt.dynamicws = false;
		SetTaskbarBlur();
	}

	CloseHandle(run.ev);
	Shell_NotifyIcon(NIM_DELETE, &run.tray);
	return 0;
}

#pragma endregion