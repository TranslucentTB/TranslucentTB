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

// for making the menu show up better
#include <ShellScalingAPI.h>

//used for the tray things
#include <shellapi.h>
#include "resource.h"

// UWP usings.
using namespace Windows::Foundation;

//we use a GUID for uniqueness
const static LPCWSTR singleProcName = L"344635E9-9AE4-4E60-B128-D53E25AB70A7";

//needed for tray exit
bool run = true;

// holds the alpha channel value between 0 or 255,
// defaults to -1 (not set).
int forcedtransparency;

HWND taskbar;
HWND secondtaskbar;
HMENU popup;

#define PEEK_DISABLED 0
#define PEEK_ENABLED  1
#define PEEK_DYNAMIC  2

bool cached_peek = true;
bool should_show_peek;

#pragma region composition

struct ACCENTPOLICY
{
	int nAccentState;
	int nFlags;
	int nColor;
	int nAnimationId;
};
struct WINCOMPATTRDATA
{
	int nAttribute;
	PVOID pData;
	ULONG ulDataSize;
};

enum TASKBARSTATE { Normal, WindowMaximised, StartMenuOpen }; // Create a state to store all
															  // states of the Taskbar
			// Normal           | Proceed as normal. If no dynamic options are set, act as it says in opt.taskbar_appearance
			// WindowMaximised  | There is a window which is maximised on the monitor this HWND is in. Display as blurred.
			// StartMenuOpen    | The Start Menu is open on the monitor this HWND is in. Display as it would be without TranslucentTB active.

enum SAVECONFIGSTATES { DoNotSave, SaveAll } shouldsaveconfig;  // Create an enum to store all config states
			// DoNotSave        | Fairly self-explanatory
			// SaveAll          | Save all options

struct TASKBARPROPERTIES
{
	HMONITOR hmon;
	TASKBARSTATE state;
};

std::vector<std::wstring> IgnoredClassNames;
std::vector<std::wstring> IgnoredExeNames;
std::vector<std::wstring> IgnoredWindowTitles;

int counter = 0;
const int ACCENT_DISABLED = 4; // Disables TTB for that taskbar
const int ACCENT_ENABLE_GRADIENT = 1; // Makes the taskbar a solid color specified by nColor. This mode doesn't care about the alpha channel.
const int ACCENT_ENABLE_TRANSPARENTGRADIENT = 2; // Makes the taskbar a tinted transparent overlay. nColor is the tint color, sending nothing results in it interpreted as 0x00000000 (totally transparent, blends in with desktop)
const int ACCENT_ENABLE_BLURBEHIND = 3; // Makes the taskbar a tinted blurry overlay. nColor is same as above.
const int ACCENT_ENABLE_TINTED = 5; // This is not a real state. We will handle it later.
const int ACCENT_NORMAL_GRADIENT = 6; // Another fake value, handles the
unsigned int WM_TASKBARCREATED;
unsigned int NEW_TTB_INSTANCE;
std::map<HWND, TASKBARPROPERTIES> taskbars; // Create a map for all taskbars

struct OPTIONS
{
	int taskbar_appearance = ACCENT_ENABLE_BLURBEHIND;
	int color = 0x00000000;
	bool dynamicws = false;
	int dynamic_ws_state = ACCENT_ENABLE_BLURBEHIND; // State to activate when d-ws is enabled
	bool dynamicstart = false;
	int peek = PEEK_ENABLED;
} opt;

IVirtualDesktopManager *desktop_manager = NULL;

typedef BOOL(WINAPI*pSetWindowCompositionAttribute)(HWND, WINCOMPATTRDATA*);
static pSetWindowCompositionAttribute SetWindowCompositionAttribute = (pSetWindowCompositionAttribute)GetProcAddress(GetModuleHandle(TEXT("user32.dll")), "SetWindowCompositionAttribute");

void SetWindowBlur(HWND hWnd, int appearance = 0) // `appearance` can be 0, which means 'follow opt.taskbar_appearance'
{
	if (SetWindowCompositionAttribute)
	{
		ACCENTPOLICY policy;

		if (appearance) // Custom taskbar appearance is set
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
			else if (opt.taskbar_appearance == ACCENT_NORMAL_GRADIENT) { policy = { ACCENT_ENABLE_TRANSPARENTGRADIENT, 2, (int)0xd9000000, 0 }; } // normal gradient color
			else { policy = { opt.taskbar_appearance, 2, opt.color, 0 }; }
		}

		WINCOMPATTRDATA data = { 19, &policy, sizeof(ACCENTPOLICY) }; // WCA_ACCENT_POLICY=19
		SetWindowCompositionAttribute(hWnd, &data);
	}
}

#pragma endregion

#pragma region IO help

bool file_exists(std::wstring path)
{
	std::ifstream infile(path);
	return infile.good();
}

#pragma endregion


#pragma region Configuration

void add_to_startup()
{
	HMODULE hModule = GetModuleHandle(NULL);
	TCHAR path[MAX_PATH];
	GetModuleFileName(hModule, path, MAX_PATH);
	std::wstring unsafePath = path;
	std::wstring progPath = L"\"" + unsafePath + L"\"";
	HKEY hkey = NULL;
	LONG createStatus = RegCreateKey(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", &hkey); //Creates a key
	LONG status = RegSetValueEx(hkey, L"TranslucentTB", 0, REG_SZ, (BYTE *)progPath.c_str(), (DWORD)((progPath.size() + 1) * sizeof(wchar_t)));
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
			value == L"translucent")
			opt.taskbar_appearance = ACCENT_ENABLE_TRANSPARENTGRADIENT;
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
		if (!opt.taskbar_appearance && opt.dynamicws) {
			opt.taskbar_appearance = ACCENT_ENABLE_TRANSPARENTGRADIENT;
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

		forcedtransparency = parsed;
	}
	else if (arg == L"peek")
	{
		if (value == L"hide")
		{
			opt.peek = PEEK_DISABLED;
		}
		else if (value == L"dynamic")
		{
			opt.peek = PEEK_DYNAMIC;
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

	if (forcedtransparency >= 0)
	{
		opt.color = (forcedtransparency << 24) +
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
		configstream << L"; File gets overwritten on exit. Only edit when TranslucentTB is not running." << endl;
		configstream << L"; ===========================================================================" << endl;
		configstream << endl;

		configstream << L"; Taskbar appearance: opaque, transparent, or blur (default)." << endl;

		if (opt.taskbar_appearance == ACCENT_ENABLE_GRADIENT)
			configstream << L"accent=opaque" << endl;
		else if (opt.taskbar_appearance == ACCENT_ENABLE_TRANSPARENTGRADIENT)
			configstream << L"accent=transparent" << endl;
		else if (opt.taskbar_appearance == ACCENT_ENABLE_BLURBEHIND)
			configstream << L"accent=blur" << endl;

		configstream << endl;
		configstream << L"; Dynamic states: Window States and (WIP) Start Menu" << endl;
		configstream << L"; dynamic windows: opaque, tint, or blur (default)." << endl;
		if (!opt.dynamicws)
			configstream << L"; ";

		if (opt.dynamic_ws_state == ACCENT_ENABLE_BLURBEHIND)
			configstream << L"dynamic-ws=blur" << endl;
		else if (opt.dynamic_ws_state == ACCENT_ENABLE_TINTED)
			configstream << L"dynamic-ws=tint" << endl;
		else if (opt.dynamic_ws_state == ACCENT_ENABLE_GRADIENT)
			configstream << L"dynamic-ws=opaque" << endl;
		else
			configstream << L"dynamic-ws=enable" << endl;

		configstream << L"; dynamic windows can be used in conjunction with a custom color and non-zero opacity!" << endl;
		configstream << L"; you can also set an accent value, which will represent the state of dynamic windows when there is no window maximised" << endl;

		if (opt.dynamicstart)
			configstream << L"dynamic-start=enable" << endl;
		else
			configstream << L"; dynamic-start=enable" << endl;

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
		configstream << L"; If this is enabled, there will be no tray icon" << endl;
		configstream << endl;
		configstream << L"; Controls how the Aero Peek button behaves" << endl;
		if (opt.peek == PEEK_DISABLED)
			configstream << L"peek=hide" << endl;
		else if (opt.peek == PEEK_DYNAMIC)
			configstream << L"peek=dynamic" << endl;
		else
			configstream << L"peek=show" << endl;
	}
}

std::wstring trim(std::wstring& str)
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
		token = trim(row.substr(0, pos));
		result.push_back(token);
		row.erase(0, pos + delimiter.length());
	}
	return result;
}

void ParseDWSExcludesFile(std::wstring filename)
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
			IgnoredClassNames = ParseByDelimiter(line, delimiter);
			IgnoredClassNames.erase(IgnoredClassNames.begin());
		}
		else if (line_lowercase.substr(0, 5) == L"title" ||
			line.substr(0, 13) == L"windowtitle")
		{
			IgnoredWindowTitles = ParseByDelimiter(line, delimiter);
			IgnoredWindowTitles.erase(IgnoredWindowTitles.begin());
		}
		else if (line_lowercase.substr(0, 7) == L"exename")
		{
			IgnoredExeNames = ParseByDelimiter(line, delimiter);
			IgnoredExeNames.erase(IgnoredExeNames.begin());
		}
	}
}

#pragma endregion

void RefreshHandles()
{
	HWND _taskbar;
	TASKBARPROPERTIES _properties;

	taskbars.clear();
	_taskbar = FindWindowW(L"Shell_TrayWnd", NULL);

	_properties.hmon = MonitorFromWindow(_taskbar, MONITOR_DEFAULTTOPRIMARY);
	_properties.state = Normal;
	taskbars.insert(std::make_pair(_taskbar, _properties));
	while (secondtaskbar = FindWindowEx(0, secondtaskbar, L"Shell_SecondaryTrayWnd", NULL))
	{
		_properties.hmon = MonitorFromWindow(secondtaskbar, MONITOR_DEFAULTTOPRIMARY);
		_properties.state = Normal;
		taskbars.insert(std::make_pair(secondtaskbar, _properties));
	}
}

#pragma region tray

#define WM_NOTIFY_TB 3141

HMENU menu;
NOTIFYICONDATA Tray;
HWND tray_hwnd;

bool CheckPopupItem(UINT item_to_check, bool state)
{
	return CheckMenuItem(popup, item_to_check, MF_BYCOMMAND | (state ? MF_CHECKED : MF_UNCHECKED) | MF_ENABLED);
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
	else if (opt.taskbar_appearance == ACCENT_NORMAL_GRADIENT)
	{
		radio_to_check_regular = IDM_NORMAL;
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
	else if (opt.dynamic_ws_state == ACCENT_ENABLE_GRADIENT)
	{
		radio_to_check_dynamic = IDM_DYNAMICWS_NORMAL;
	}
	else
	{
		OutputDebugString(L"Unable to determine which radio item to check for dynamic state!");
	}



	if (opt.peek == PEEK_ENABLED)
	{
		radio_to_check_peek = IDM_PEEK;
	}
	else if (opt.peek == PEEK_DYNAMIC)
	{
		radio_to_check_peek = IDM_DPEEK;
	}
	else if (opt.peek == PEEK_DISABLED)
	{
		radio_to_check_peek = IDM_NOPEEK;
	}
	else
	{
		OutputDebugString(L"Unable to determine which radio item to check for peek!");
	}

	CheckMenuRadioItem(popup, IDM_BLUR, IDM_NORMAL, radio_to_check_regular, MF_BYCOMMAND);
	CheckMenuRadioItem(popup, IDM_DYNAMICWS_BLUR, IDM_DYNAMICWS_NORMAL, radio_to_check_dynamic, MF_BYCOMMAND);
	CheckMenuRadioItem(popup, IDM_PEEK, IDM_NOPEEK, radio_to_check_peek, MF_BYCOMMAND);

	INT items_to_enable[] = { IDM_DYNAMICWS_BLUR, IDM_DYNAMICWS_CLEAR, IDM_DYNAMICWS_NORMAL };
	for (INT item : items_to_enable)
		EnableMenuItem(popup, item, MF_BYCOMMAND | (opt.dynamicws ? MF_ENABLED : MF_GRAYED));

	CheckPopupItem(IDM_DYNAMICWS, opt.dynamicws);
	CheckPopupItem(IDM_DYNAMICSTART, opt.dynamicstart);
	CheckPopupItem(IDM_AUTOSTART, RegGetValue(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", L"TranslucentTB", RRF_RT_REG_SZ, NULL, NULL, NULL) == ERROR_SUCCESS);
}

void initTray(HWND parent)
{
	Tray.cbSize = sizeof(Tray);
	Tray.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(MAINICON));
	Tray.hWnd = parent;
	wcscpy_s(Tray.szTip, L"TranslucentTB");
	Tray.uCallbackMessage = WM_NOTIFY_TB;
	Tray.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
	Tray.uID = 101;
	Shell_NotifyIcon(NIM_ADD, &Tray);
	Shell_NotifyIcon(NIM_SETVERSION, &Tray);
	RefreshMenu();
}

bool isBlacklisted(HWND hWnd)
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
	for (auto & value : IgnoredClassNames)
	{
		if (className == value.c_str()) { return true; }
	}
	for (auto & value : IgnoredExeNames)
	{
		if (exeName == value) { return true; }
	}
	for (auto & value : IgnoredWindowTitles)
	{
		if (w_WindowTitle.find(value) != std::wstring::npos)
		{
			return true;
		}
	}
	return false;
}

void TogglePeek(bool status)
{
	if (status != cached_peek)
	{
		HWND _taskbar = FindWindow(L"Shell_TrayWnd", NULL);
		HWND _tray = FindWindowEx(_taskbar, NULL, L"TrayNotifyWnd", NULL);
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

BOOL CALLBACK EnumWindowsProcess(HWND hWnd, LPARAM lParam)
{
	HMONITOR _monitor;

	if (opt.dynamicws || opt.peek == PEEK_DYNAMIC)
	{
		WINDOWPLACEMENT result = {};
		::GetWindowPlacement(hWnd, &result);
		if (result.showCmd == SW_MAXIMIZE) {
			BOOL on_current_desktop = true;
			if (desktop_manager)
				desktop_manager->IsWindowOnCurrentVirtualDesktop(hWnd, &on_current_desktop);
			if (IsWindowVisible(hWnd) && on_current_desktop)
			{
				if (!isBlacklisted(hWnd))
				{
					_monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTOPRIMARY);
					HWND main_taskbar = FindWindow(L"Shell_TrayWnd", NULL);
					for (auto &taskbar : taskbars)
					{
						if (taskbar.first == main_taskbar &&
							taskbar.second.hmon == _monitor &&
							opt.peek == PEEK_DYNAMIC)
						{
							should_show_peek = true;
						}

						if (taskbar.second.hmon == _monitor &&
							taskbar.second.state != StartMenuOpen)
						{
							taskbar.second.state = WindowMaximised;
						}
					}
					
				}
			}
		}
	}
	return true;
}

LRESULT CALLBACK TBPROCWND(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CLOSE:
		PostQuitMessage(0);
		break;
	case WM_NOTIFY_TB:
		if (lParam == WM_LBUTTONUP || lParam == WM_RBUTTONUP)
		{
			POINT pt;
			GetCursorPos(&pt);
			SetForegroundWindow(hWnd);
			UINT tray = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, pt.x, pt.y, 0, hWnd, NULL);
			switch (tray) // TODO: Add menu items for colors, and one to open config file locations, and one to reset ApplicationFrameHost (after reset if there is still a bad window, show details to help in bug report)
			{
			case IDM_BLUR:
				opt.taskbar_appearance = ACCENT_ENABLE_BLURBEHIND;
				break;
			case IDM_CLEAR:
				opt.taskbar_appearance = ACCENT_ENABLE_TRANSPARENTGRADIENT;
				break;
			case IDM_NORMAL:
				opt.taskbar_appearance = ACCENT_NORMAL_GRADIENT;
				break;
			case IDM_DYNAMICWS:
				opt.dynamicws = !opt.dynamicws;
				EnumWindows(&EnumWindowsProcess, NULL);
				break;
			case IDM_DYNAMICWS_BLUR:
				opt.dynamic_ws_state = ACCENT_ENABLE_BLURBEHIND;
				break;
			case IDM_DYNAMICWS_CLEAR:
				opt.dynamic_ws_state = ACCENT_ENABLE_TINTED;
				break;
			case IDM_DYNAMICWS_NORMAL:
				opt.dynamic_ws_state = ACCENT_ENABLE_GRADIENT;
				break;
			case IDM_DYNAMICSTART:
				opt.dynamicstart = !opt.dynamicstart;
				break;
			case IDM_PEEK:
				opt.peek = PEEK_ENABLED;
				break;
			case IDM_DPEEK:
				opt.peek = PEEK_DYNAMIC;
				break;
			case IDM_NOPEEK:
				opt.peek = PEEK_DISABLED;
				break;
			case IDM_AUTOSTART: // TODO: Use UWP Apis
				if (RegGetValue(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", L"TranslucentTB", RRF_RT_REG_SZ, NULL, NULL, NULL) == ERROR_SUCCESS)
				{
					HKEY hkey = NULL;
					RegCreateKey(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", &hkey);
					RegDeleteValue(hkey, L"TranslucentTB");
				}
				else {
					add_to_startup();
				}
				break;
			case IDM_EXIT:
				run = false;
				break;
			}
			RefreshMenu();
		}
	}
	if (message == WM_TASKBARCREATED) // Unfortunately, WM_TASKBARCREATED is not a constant, so I can't include it in the switch.
	{
		RefreshHandles();
		initTray(tray_hwnd);
	}
	else if (message == NEW_TTB_INSTANCE) {
		shouldsaveconfig = DoNotSave;
		run = false;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

void SetTaskbarBlur()
{
	// std::cout << opt.dynamicws << std::endl;

	if (counter >= 10)   // Change this if you want to change the time it takes for the program to update
	{                   // 100 = 1 second; we use 10, because the difference is less noticeable and it has
						// no large impact on CPU. We can change this if we feel that CPU is more important
						// than response time.
		should_show_peek = (opt.peek == PEEK_ENABLED);

		for (auto &taskbar : taskbars)
		{
			taskbar.second.state = Normal; // Reset taskbar state
		}
		if (opt.dynamicws || opt.peek == PEEK_DYNAMIC) {
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
				for (auto &taskbar : taskbars)
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

	for (auto const &taskbar : taskbars)
	{
		if (taskbar.second.state == WindowMaximised) {
			SetWindowBlur(taskbar.first, opt.dynamic_ws_state);
			// A window is maximised; let's make sure that we blur the window.
		}
		else if (taskbar.second.state == Normal) {
			SetWindowBlur(taskbar.first);  // Taskbar should be normal, call using normal transparency settings
		}
	}
	TogglePeek(should_show_peek);
	counter++;
}

#pragma endregion

HANDLE ev;

bool singleProc()
{
	ev = CreateEvent(NULL, TRUE, FALSE, singleProcName);
	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		return false;
	}
	return true;
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPreInst, _In_ LPSTR pCmdLine, _In_ int nCmdShow)
{
	if (FAILED(SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE))) { OutputDebugStringW(L"Per-monitor DPI scaling failed\n"); }

	LPWSTR localAppData;

	if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_DEFAULT, NULL, &localAppData)))
	{
		MessageBox(NULL, L"Failed to get LocalAppData folder location!\n\nProgram will exit.", L"TranslucentTB - Fatal error", MB_ICONERROR | MB_OK);
		return 1;
	}

	TCHAR configFolder[MAX_PATH];
	TCHAR excludeFile[MAX_PATH];
	TCHAR configFile[MAX_PATH];
	TCHAR exeFolder[MAX_PATH];
	TCHAR stockConfigFile[MAX_PATH];
	TCHAR stockExcludeFile[MAX_PATH];

	PathCombine(configFolder, localAppData, L"TranslucentTB");
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

	ParseConfigFile(configFile); // Config file settings
	ParseDWSExcludesFile(excludeFile);

	shouldsaveconfig = SaveAll;

	NEW_TTB_INSTANCE = RegisterWindowMessage(L"NewTTBInstance");
	if (!singleProc()) {
		HWND oldInstance = FindWindow(L"TranslucentTB", L"TrayWindow");
		SendMessage(oldInstance, NEW_TTB_INSTANCE, NULL, NULL);
	}

	MSG msg; // for message translation and dispatch
	popup = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_POPUP_MENU));
	menu = GetSubMenu(popup, 0);
	WNDCLASSEX wnd = { 0 };

	wnd.hInstance = hInstance;
	wnd.lpszClassName = L"TranslucentTB";
	wnd.lpfnWndProc = TBPROCWND;
	wnd.style = CS_HREDRAW | CS_VREDRAW;
	wnd.cbSize = sizeof(WNDCLASSEX);

	wnd.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wnd.hCursor = LoadCursor(NULL, IDC_ARROW);
	wnd.hbrBackground = (HBRUSH)BLACK_BRUSH;
	RegisterClassEx(&wnd);

	tray_hwnd = CreateWindowEx(WS_EX_TOOLWINDOW, L"TranslucentTB", L"TrayWindow", WS_OVERLAPPEDWINDOW, 0, 0,
		400, 400, NULL, NULL, hInstance, NULL);

	initTray(tray_hwnd);

	ShowWindow(tray_hwnd, WM_SHOWWINDOW);

	// Store stuff
	if (FAILED(Initialize())) { OutputDebugStringW(L"Initialization of UWP APIs failed. Unable to manipulate startup entry."); }
	//Windows::ApplicationModel::StartupTask::GetForCurrentPackageAsync()->GetResults()->GetAt(1)->State;
	// This should not make it crash once it gets packaged into an APPX.

	//Virtual Desktop stuff
	if (FAILED(::CoInitialize(NULL))) { OutputDebugStringW(L"Initialization of COM failed, VirtualDesktopManager will probably fail too.\n"); }
	HRESULT desktop_success = ::CoCreateInstance(__uuidof(VirtualDesktopManager), NULL, CLSCTX_INPROC_SERVER, IID_IVirtualDesktopManager, (void **)&desktop_manager);
	if (FAILED(desktop_success)) { OutputDebugStringW(L"Initialization of VirtualDesktopManager failed, dynamic windows will not support Windows virtual desktops.\n"); }

	RefreshHandles();
	if (opt.dynamicws)
	{
		EnumWindows(&EnumWindowsProcess, NULL); // Putting this here so there isn't a
												// delay between when you start the
												// program and when the taskbar goes blurry
	}
	WM_TASKBARCREATED = RegisterWindowMessage(L"TaskbarCreated");

	while (run) {
		if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		SetTaskbarBlur();
		Sleep(10);
	}
	Shell_NotifyIcon(NIM_DELETE, &Tray);

	if (shouldsaveconfig != DoNotSave)
		SaveConfigFile(configFile);

	opt.taskbar_appearance = ACCENT_NORMAL_GRADIENT;
	TogglePeek(true);
	SetTaskbarBlur();
	CloseHandle(ev);
	return 0;
}