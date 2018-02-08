#pragma region Includes

// Standard API
#include <algorithm>
#include <cwctype>
#include <fstream>
#include <iomanip>
#include <unordered_map>
#include <string>
#include <vector>

// Windows API
#include <comdef.h>
#include <dwmapi.h>
#include <psapi.h>
#include <roapi.h>
#include <shellapi.h>
#include <ShellScalingAPI.h>
#include <ShlObj.h>
#include <Shlwapi.h>
#include <tchar.h>
#include <windows.h>

// Local
#include "resource.h"

// Libraries
#include "../CPicker/CPicker.h"

#pragma endregion

using namespace Windows::Foundation;

#pragma region Enumerations

enum TASKBARSTATE {		// enum to store states of a taskbar
	Normal,				// Proceed as normal. If no dynamic options are set, act as it says in opt.taskbar_appearance
	WindowMaximised,	// There is a window which is maximised on the monitor this HWND is in. Display as blurred.
	StartMenuOpen		// The Start Menu is open on the monitor this HWND is in. Display as it would be without TranslucentTB active.
};

enum EXITREASON {		// enum to store possible exit reasons
	NewInstance,		// New instance told us to exit
	UserAction,			// Triggered by the user
	UserActionNoSave	// Triggered by the user, but doesn't saves config
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
	bool dynamicws = false;										// Whether dynamic windows are enabled
	ACCENTSTATE dynamic_ws_state = ACCENT_ENABLE_BLURBEHIND;	// State to activate when a window is maximised
	bool dynamicws_peek = true;									// Whether to use the normal style when using Aero Peek
	bool dynamicstart = false;									// Whether dynamic start is enabled
	PEEKSTATE peek = Enabled;									// Controls how the Aero Peek button is handled
	std::vector<std::wstring> blacklisted_classes;				// List of window classes the user blacklisted
	std::vector<std::wstring> blacklisted_filenames;			// List of executable filenames the user blacklisted
	std::vector<std::wstring> blacklisted_titles;				// List of window titles the user blacklisted
} opt;

static struct RUNTIME															// Used to store things relevant only to runtime
{
	EXITREASON exit_reason = UserAction;										// Determines if current configuration should be saved when we exit
	IVirtualDesktopManager *desktop_manager = NULL;								// Used to detect if a window is in the current virtual desktop. Don't forget to check for null on this one
	IAppVisibility *app_visibility = NULL;										// Used to detect if start menu is opened
	HWND main_taskbar;															// Handle to taskbar that shows on main monitor
	std::unordered_map<HWND, TASKBARPROPERTIES> taskbars;						// Map for all taskbars and their properties
	bool should_show_peek;														// Set by the EnumWindowsProcess and determines if peek should be shown when in dynamic mode
	bool run = true;															// Set to false to break out of the main program loop
	HMENU popup;																// Tray icon popup
	HANDLE ev;																	// Handle to an event. Used for uniqueness
	NOTIFYICONDATA tray;														// Tray icon
	HWND tray_hwnd;																// Tray window handle
	bool fluent_available = false;												// Whether ACCENT_ENABLE_FLUENT works
	TCHAR config_folder[MAX_PATH];												// Folder where configuration is stored
	TCHAR config_file[MAX_PATH];												// Location of configuration file
	TCHAR exclude_file[MAX_PATH];												// Location of blacklist file
	int cache_hits;																// Number of times the blacklist cache has been hit
	bool peek_active = false;													// Determines if the user is currently peeking the desktop
} run;

const static struct CONSTANTS												// Constants. What else do you need?
{
	LPCWSTR guid = L"344635E9-9AE4-4E60-B128-D53E25AB70A7";					// Used to prevent two instances running at the same time
	UINT WM_NOTIFY_TB = 3141;												// Message id for tray callback
	LPCWSTR program_name = L"TranslucentTB";								// Sounds weird, but prevents typos
	UINT WM_TASKBARCREATED = RegisterWindowMessage(L"TaskbarCreated");		// Message received when Explorer restarts
	UINT NEW_TTB_INSTANCE = RegisterWindowMessage(L"NewTTBInstance");		// Message sent when an instance should exit because a new one started
	std::unordered_map<ACCENTSTATE, UINT> normal_button_map = {				// Holds a map of which setting is associated to which button
		{ ACCENT_ENABLE_BLURBEHIND,				IDM_BLUR },
		{ ACCENT_ENABLE_TRANSPARENTGRADIENT,	IDM_CLEAR },
		{ ACCENT_NORMAL,						IDM_NORMAL },
		{ ACCENT_ENABLE_GRADIENT,				IDM_OPAQUE },
		{ ACCENT_ENABLE_FLUENT,					IDM_FLUENT }
	};
	std::unordered_map<ACCENTSTATE, UINT> dynamic_button_map = {			// Holds a map of which setting is associated to which button
		{ ACCENT_ENABLE_BLURBEHIND,				IDM_DYNAMICWS_BLUR },
		{ ACCENT_ENABLE_TINTED,					IDM_DYNAMICWS_CLEAR },
		{ ACCENT_NORMAL,						IDM_DYNAMICWS_NORMAL },
		{ ACCENT_ENABLE_GRADIENT,				IDM_DYNAMICWS_OPAQUE },
		{ ACCENT_ENABLE_FLUENT,					IDM_DYNAMICWS_FLUENT }
	};
	std::unordered_map<PEEKSTATE, UINT> peek_button_map = {					// Holds a map of which setting is associated to which button
		{ Disabled,		IDM_NOPEEK },
		{ Dynamic,		IDM_DPEEK },
		{ Enabled,		IDM_PEEK }
	};
	LPWSTR config_file = L"config.cfg";										// Name of configuration file
	LPWSTR exclude_file = L"dynamic-ws-exclude.csv";						// Name of dynamic windows blacklist file
	int max_cache_hits = 500;												// Maximum number of times the blacklist cache may be hit
} cnst;

#pragma endregion

#pragma region That one function that does all the magic

void SetWindowBlur(HWND hWnd, ACCENTSTATE appearance = ACCENT_FOLLOW_OPT)
{
	typedef BOOL(WINAPI *pSetWindowCompositionAttribute)(HWND, WINCOMPATTRDATA *);
	static pSetWindowCompositionAttribute SetWindowCompositionAttribute = reinterpret_cast<pSetWindowCompositionAttribute>(GetProcAddress(GetModuleHandle(L"user32.dll"), "SetWindowCompositionAttribute"));

	if (SetWindowCompositionAttribute)
	{
		ACCENTPOLICY policy;
		UINT color = (opt.color & 0xFF00FF00) + ((opt.color & 0x00FF0000) >> 16) + ((opt.color & 0x000000FF) << 16);

		if (appearance != ACCENT_FOLLOW_OPT) // Custom taskbar appearance is set
		{
			if (appearance == ACCENT_ENABLE_TINTED) // Window is maximised
			{
				policy = { ACCENT_ENABLE_TRANSPARENTGRADIENT, 2, color, 0 };
			}
			else if (appearance == ACCENT_NORMAL)
			{
				policy = { (run.fluent_available ? ACCENT_ENABLE_FLUENT : ACCENT_ENABLE_TRANSPARENTGRADIENT), 2, 0x99000000, 0 };
			}
			else
			{
				policy = { appearance, 2, opt.color, 0 };
			}
		}
		else // Use the defaults
		{
			if (opt.dynamic_ws_state == ACCENT_ENABLE_TINTED) // dynamic-ws is tint and desktop is shown
			{
				policy = { ACCENT_ENABLE_TRANSPARENTGRADIENT, 2, 0x00000000, 0 };
			}
			else if (opt.taskbar_appearance == ACCENT_NORMAL) // normal gradient color
			{
				policy = { (run.fluent_available ? ACCENT_ENABLE_FLUENT : ACCENT_ENABLE_TRANSPARENTGRADIENT), 2, 0x99000000, 0 };
			}
			else
			{
				policy = { opt.taskbar_appearance, 2, color, 0 };
			}
		}

		WINCOMPATTRDATA data = { WCA_ACCENT_POLICY, &policy, sizeof(policy) };
		SetWindowCompositionAttribute(hWnd, &data);
	}
}

#pragma endregion

#pragma region String manipulation

void ToLower(std::wstring &data)
{
	std::transform(data.begin(), data.end(), data.begin(), ::towlower);
}

std::wstring Trim(const std::wstring& str)
{
	size_t first = str.find_first_not_of(' ');
	size_t last = str.find_last_not_of(' ');

	if (first == std::wstring::npos)
	{
		return std::wstring(L"");
	}
	return str.substr(first, (last - first + 1));
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

	PathCombine(run.config_folder, localAppData, cnst.program_name);
	PathCombine(run.config_file, run.config_folder, cnst.config_file);
	PathCombine(run.exclude_file, run.config_folder, cnst.exclude_file);

	return ERROR_SUCCESS;
}

void ApplyStock(LPWSTR filename)
{
	TCHAR exeFolder[MAX_PATH];
	GetModuleFileName(GetModuleHandle(NULL), exeFolder, MAX_PATH);
	PathRemoveFileSpec(exeFolder);

	TCHAR stockFile[MAX_PATH];
	PathCombine(stockFile, exeFolder, filename);

	TCHAR configFile[MAX_PATH];
	PathCombine(configFile, run.config_folder, filename);

	if (!PathIsDirectory(run.config_folder))
	{
		CreateDirectory(run.config_folder, NULL);
	}

	CopyFile(stockFile, configFile, FALSE);
}

void CheckAndRunWelcome()
{
	if (!PathIsDirectory(run.config_folder))
	{
		// String concatenation is hard OK
		std::wstring message;
		message += L"Welcome to ";
		message += cnst.program_name;
		message += L"!\n\n";
		message += L"You can tweak the taskbar's appearance with the tray icon. If it's your cup of tea, you can also edit the configuration files, located at \"";
		message += run.config_folder;
		message += '"';

		MessageBox(NULL, message.c_str(), std::wstring(cnst.program_name).c_str(), MB_ICONINFORMATION | MB_OK);
	}
	if (!PathFileExists(run.config_file))
	{
		ApplyStock(cnst.config_file);
	}
	if (!PathFileExists(run.exclude_file))
	{
		ApplyStock(cnst.exclude_file);
	}
}

bool GetStartupState()
{
	// SUCCEEDED macro considers ERROR_FILE_NOT_FOUND as succeeded ???
	return RegGetValue(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", cnst.program_name, RRF_RT_REG_SZ, NULL, NULL, NULL) == ERROR_SUCCESS;
}

void SetStartupState(bool state)
{
	HKEY hkey;
	if (SUCCEEDED(RegCreateKey(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", &hkey))) //Creates a key
	{
		if (state)
		{
			HMODULE hModule = GetModuleHandle(NULL);
			TCHAR path[MAX_PATH];
			GetModuleFileName(hModule, path, MAX_PATH);
			PathQuoteSpaces(path);

			RegSetValueEx(hkey, cnst.program_name, 0, REG_SZ, reinterpret_cast<BYTE *>(path), _tcslen(path) * sizeof(TCHAR));
		}
		else
		{
			RegDeleteValue(hkey, cnst.program_name);
		}
		RegCloseKey(hkey);
	}
}

void ParseSingleConfigOption(std::wstring arg, std::wstring value)
{
	if (arg == L"accent")
	{
		if (value == L"blur")
		{
			opt.taskbar_appearance = ACCENT_ENABLE_BLURBEHIND;
		}
		else if (value == L"opaque")
		{
			opt.taskbar_appearance = ACCENT_ENABLE_GRADIENT;
		}
		else if (value == L"transparent" || value == L"translucent" || value == L"clear")
		{
			opt.taskbar_appearance = ACCENT_ENABLE_TRANSPARENTGRADIENT;
		}
		else if (value == L"normal")
		{
			opt.taskbar_appearance = ACCENT_NORMAL;
		}
		else if (value == L"fluent" && run.fluent_available)
		{
			opt.taskbar_appearance = ACCENT_ENABLE_FLUENT;
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
		if (value == L"true" || value == L"enable")
		{
			opt.dynamicstart = true;
		}
	}
	else if (arg == L"color" || arg == L"tint")
	{
		value = Trim(value);

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
			opt.peek = Disabled;
		}
		else if (value == L"dynamic")
		{
			opt.peek = Dynamic;
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
		case ACCENT_ENABLE_GRADIENT:
			configstream << L"opaque";
			break;
		case ACCENT_ENABLE_TRANSPARENTGRADIENT:
			configstream << L"clear";
			break;
		case ACCENT_ENABLE_BLURBEHIND:
			configstream << L"blur";
			break;
		case ACCENT_NORMAL:
			configstream << L"normal";
			break;
		case ACCENT_ENABLE_FLUENT:
			configstream << L"fluent";
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
		case ACCENT_ENABLE_BLURBEHIND:
			configstream << L"blur";
			break;
		case ACCENT_ENABLE_TINTED:
			configstream << L"tint";
			break;
		case ACCENT_ENABLE_GRADIENT:
			configstream << L"opaque";
			break;
		case ACCENT_NORMAL:
			configstream << L"normal";
			break;
		case ACCENT_ENABLE_FLUENT:
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
		case Disabled:
			configstream << L"hide";
			break;
		case Dynamic:
			configstream << L"dynamic";
			break;
		default:
			configstream << L"show";
			break;
		}
		configstream << endl;
	}
}

void AddValuesToVectorByDelimiter(std::wstring delimiter, std::vector<std::wstring> &vector, std::wstring line)
{
	size_t pos;
	std::wstring value;

	// First lets remove the key
	if ((pos = line.find(delimiter)) != std::wstring::npos)
	{
		line.erase(0, pos + delimiter.length());
	}

	// Now iterate and add the values
	while ((pos = line.find(delimiter)) != std::wstring::npos)
	{
		value = Trim(line.substr(0, pos));
		vector.push_back(value);
		line.erase(0, pos + delimiter.length());
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
		ToLower(line_lowercase);

		if (line_lowercase.substr(0, 5) == L"class")
		{
			AddValuesToVectorByDelimiter(delimiter, opt.blacklisted_classes, line);
		}
		else if (line_lowercase.substr(0, 5) == L"title" || line.substr(0, 13) == L"windowtitle")
		{
			AddValuesToVectorByDelimiter(delimiter, opt.blacklisted_titles, line);
		}
		else if (line_lowercase.substr(0, 7) == L"exename")
		{
			AddValuesToVectorByDelimiter(delimiter, opt.blacklisted_filenames, line_lowercase);
		}
	}
}

void EditFile(std::wstring file)
{
	// WinAPI reeeeeeeeeeeeeeeeeeeeeeeeee
	LPWSTR system32;
	HRESULT error = SHGetKnownFolderPath(FOLDERID_System, KF_FLAG_DEFAULT, NULL, &system32);
	if (FAILED(error))
	{
		std::wstring message;
		message += L"Failed to determine System32 folder location!\n\nException from HRESULT: ";
		message += _com_error(error).ErrorMessage();

		MessageBox(NULL, message.c_str(), (std::wstring(cnst.program_name) + L" - Fatal error").c_str(), MB_ICONERROR | MB_OK);

		return;
	}

	TCHAR notepad[MAX_PATH];
	PathCombine(notepad, system32, L"notepad.exe");

	std::vector<TCHAR> buf(file.begin(), file.end());
	buf.push_back(0); // Null terminator
	PathQuoteSpaces(buf.data());

	std::wstring path;
	path += notepad;
	path += ' ';
	path += buf.data();

	std::vector<TCHAR> buf2(path.begin(), path.end());
	buf2.push_back(0); // Null terminator
	STARTUPINFO si = { sizeof(si) };
	PROCESS_INFORMATION pi;
	// Not using lpApplicationName here because if someone has set a redirect to another editor it doesn't works. (eg Notepad2)
	if (CreateProcess(NULL, buf2.data(), NULL, NULL, FALSE, NULL, NULL, NULL, &si, &pi))
	{
		WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}
	else
	{
		error = GetLastError();
		std::wstring message;
		message += L"Failed to start Notepad!\n\nException from HRESULT: ";
		message += _com_error(error).ErrorMessage();

		MessageBox(NULL, message.c_str(), (std::wstring(cnst.program_name) + L" - Fatal error").c_str(), MB_ICONERROR | MB_OK);
	}
}

#pragma endregion

#pragma region Utilities

void RefreshHandles()
{
	HWND secondtaskbar = NULL;
	TASKBARPROPERTIES _properties;

	// Older handles are invalid, so clear the map to be ready for new ones
	run.taskbars.clear();

	run.main_taskbar = FindWindow(L"Shell_TrayWnd", NULL);
	_properties.hmon = MonitorFromWindow(run.main_taskbar, MONITOR_DEFAULTTOPRIMARY);
	_properties.state = Normal;
	run.taskbars.insert(std::make_pair(run.main_taskbar, _properties));

	while ((secondtaskbar = FindWindowEx(0, secondtaskbar, L"Shell_SecondaryTrayWnd", NULL)) != 0)
	{
		_properties.hmon = MonitorFromWindow(secondtaskbar, MONITOR_DEFAULTTOPRIMARY);
		_properties.state = Normal;
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
	run.cache_hits = cnst.max_cache_hits + 1;
}

bool IsWindowBlacklisted(HWND hWnd)
{
	static std::unordered_map<HWND, bool> blacklist_cache;

	if (run.cache_hits <= cnst.max_cache_hits && blacklist_cache.count(hWnd) > 0)
	{
		run.cache_hits++;
		return blacklist_cache[hWnd];
	}
	else
	{
		if (run.cache_hits > cnst.max_cache_hits)
		{
			run.cache_hits = 0;
			blacklist_cache.clear();
		}

		// This is the fastest because we do the less string manipulation, so always try it first
		if (opt.blacklisted_classes.size() > 0)
		{
			TCHAR className[MAX_PATH];
			GetClassName(hWnd, className, _countof(className));
			std::wstring classNameString(className);
			for (const std::wstring &value : opt.blacklisted_classes)
			{
				if (classNameString == value)
				{
					#pragma warning(suppress: 6282)
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
			std::vector<TCHAR> windowTitleBuffer(titleSize);
			GetWindowText(hWnd, windowTitleBuffer.data(), titleSize);
			std::wstring windowTitle = windowTitleBuffer.data();

			for (const std::wstring &value : opt.blacklisted_titles)
			{
				if (windowTitle.find(value) != std::wstring::npos)
				{
					#pragma warning(suppress: 6282)
					return blacklist_cache[hWnd] = true;
				}
			}
		}

		// GetModuleFileNameEx is quite expensive according to the tracing tools, so use it as last resort.
		if (opt.blacklisted_filenames.size() > 0)
		{
			DWORD ProcessId;
			GetWindowThreadProcessId(hWnd, &ProcessId);

			TCHAR exeName_path[MAX_PATH];
			GetModuleFileNameEx(OpenProcess(PROCESS_QUERY_INFORMATION, false, ProcessId), NULL, exeName_path, _countof(exeName_path));

			std::wstring exeName = PathFindFileName(exeName_path);
			ToLower(exeName);

			for (const std::wstring &value : opt.blacklisted_filenames)
			{
				if (exeName == value)
				{
					#pragma warning(suppress: 6282)
					return blacklist_cache[hWnd] = true;
				}
			}
		}

		#pragma warning(suppress: 6282)
		return blacklist_cache[hWnd] = false;
	}
}

bool IsWindowOnCurrentDesktop(HWND hWnd)
{
	BOOL on_current_desktop;
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
	run.ev = CreateEvent(NULL, TRUE, FALSE, cnst.guid);
	return GetLastError() != ERROR_ALREADY_EXISTS;
}

bool IsAtLeastBuildNumber(unsigned int buildNumber)
{
	// Importing a driver-specific function because it's the easiest way to acquire the current OS version without being lied to

	typedef NTSTATUS(__stdcall *pRtlGetVersion)(PRTL_OSVERSIONINFOW);
	static pRtlGetVersion RtlGetVersion = reinterpret_cast<pRtlGetVersion>(GetProcAddress(GetModuleHandle(L"ntdll.dll"), "RtlGetVersion")); // Using static here shuts up code analysis /shrug
	if (RtlGetVersion)
	{
		RTL_OSVERSIONINFOW versionInfo;
		RtlGetVersion(&versionInfo);
		return versionInfo.dwBuildNumber >= buildNumber;
	}
	else
	{
		return false;
	}
}

#pragma endregion

#pragma region Tray

DWORD CheckPopupItem(UINT item_to_check, bool state)
{
	return CheckMenuItem(run.popup, item_to_check, MF_BYCOMMAND | (state ? MF_CHECKED : MF_UNCHECKED) | MF_ENABLED);
}

bool EnablePopupItem(UINT item_to_enable, bool state)
{
	return EnableMenuItem(run.popup, item_to_enable, MF_BYCOMMAND | (state ? MF_ENABLED : MF_GRAYED));
}

bool CheckPopupRadioItem(UINT from, UINT to, UINT item_to_check)
{
	return CheckMenuRadioItem(run.popup, from, to, item_to_check, MF_BYCOMMAND);
}

void RefreshMenu()
{
	// This block of CheckPopupRadioItem might throw, but if that happens we just need to update the map, or something really fucked up happened
	CheckPopupRadioItem(IDM_BLUR, IDM_FLUENT, cnst.normal_button_map.at(opt.taskbar_appearance));
	CheckPopupRadioItem(IDM_DYNAMICWS_BLUR, IDM_DYNAMICWS_FLUENT, cnst.dynamic_button_map.at(opt.dynamic_ws_state));
	CheckPopupRadioItem(IDM_PEEK, IDM_NOPEEK, cnst.peek_button_map.at(opt.peek));

	for (const UINT &item : { IDM_DYNAMICWS_BLUR, IDM_DYNAMICWS_CLEAR, IDM_DYNAMICWS_NORMAL, IDM_DYNAMICWS_OPAQUE, IDM_DYNAMICWS_PEEK })
		EnablePopupItem(item, opt.dynamicws);

	EnablePopupItem(IDM_FLUENT, run.fluent_available);
	EnablePopupItem(IDM_DYNAMICWS_FLUENT, opt.dynamicws && run.fluent_available);
	CheckPopupItem(IDM_DYNAMICWS_PEEK, opt.dynamicws_peek);
	CheckPopupItem(IDM_DYNAMICWS, opt.dynamicws);
	CheckPopupItem(IDM_DYNAMICSTART, opt.dynamicstart);
	CheckPopupItem(IDM_AUTOSTART, GetStartupState());
}

void RegisterTray()
{
	Shell_NotifyIcon(NIM_ADD, &run.tray);
	Shell_NotifyIcon(NIM_SETVERSION, &run.tray);
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
			RefreshMenu();
			POINT pt;
			GetCursorPos(&pt);
			SetForegroundWindow(hWnd);
			UINT tray = TrackPopupMenu(GetSubMenu(run.popup, 0), TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, pt.x, pt.y, 0, hWnd, NULL);
			switch (tray) // TODO: Add dynamic windows ACCENT_ENABLE_TRANSPARENT_GRADIENT
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
			case IDM_DYNAMICWS_PEEK:
				opt.dynamicws_peek = !opt.dynamicws_peek;
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
				opt.peek = Enabled;
				break;
			case IDM_DPEEK:
				opt.peek = Dynamic;
				break;
			case IDM_NOPEEK:
				opt.peek = Disabled;
				break;
			case IDM_AUTOSTART: // TODO: Use UWP Apis
				SetStartupState(!GetStartupState());
				break;
			case IDM_RETURNTODEFAULTSETTINGS:
				ApplyStock(cnst.config_file);
			case IDM_RELOADSETTINGS:
				ParseConfigFile();
				break;
			case IDM_EDITSETTINGS:
				SaveConfigFile();
				EditFile(run.config_file);
				ParseConfigFile();
				break;
			case IDM_RETURNTODEFAULTBLACKLIST:
				ApplyStock(cnst.exclude_file);
			case IDM_RELOADDYNAMICBLACKLIST:
				ParseBlacklistFile();
				ClearBlacklistCache();
				break;
			case IDM_EDITDYNAMICBLACKLIST:
				EditFile(run.exclude_file);
				ParseBlacklistFile();
				ClearBlacklistCache();
				break;
			case IDM_EXITWITHOUTSAVING:
				run.exit_reason = UserActionNoSave;
				run.run = false;
				break;
			case IDM_EXIT:
				run.run = false;
				break;
			}
		}
	}
	else if (message == cnst.WM_TASKBARCREATED)
	{
		RefreshHandles();
		RegisterTray();
	}
	else if (message == cnst.NEW_TTB_INSTANCE) {
		run.exit_reason = NewInstance;
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
		for (std::pair<const HWND, TASKBARPROPERTIES> &taskbar : run.taskbars)
		{
			if (taskbar.second.hmon == _monitor)
			{
				if (opt.dynamicws)
				{
					taskbar.second.state = WindowMaximised;
				}

				if (opt.peek == Dynamic && taskbar.first == run.main_taskbar)
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
		run.should_show_peek = (opt.peek == Enabled);

		for (std::pair<const HWND, TASKBARPROPERTIES> &taskbar : run.taskbars)
		{
			taskbar.second.state = Normal; // Reset taskbar state
		}
		if (opt.dynamicws || opt.peek == Dynamic)
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
				for (std::pair<const HWND, TASKBARPROPERTIES> &taskbar : run.taskbars)
				{
					if (taskbar.second.hmon == monitor)
					{
						taskbar.second.state = StartMenuOpen;
						break; // Useless to continue looping, we found what we desired.
					}
				}
			}
		}

		if (opt.dynamicws && opt.dynamicws_peek && run.peek_active)
		{
			for (std::pair<const HWND, TASKBARPROPERTIES> &taskbar : run.taskbars)
			{
				taskbar.second.state = Normal;
			}
		}
	}

	for (const std::pair<HWND, TASKBARPROPERTIES> &taskbar : run.taskbars)
	{
		switch (taskbar.second.state)
		{
		case StartMenuOpen:
			SetWindowBlur(taskbar.first, ACCENT_NORMAL);
			break;
		case WindowMaximised:
			SetWindowBlur(taskbar.first, opt.dynamic_ws_state); // A window is maximised; let's make sure that we blur the taskbar.
			break;
		case Normal:
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

	// Now declared in the manifest, no need to do it programatically
	//if (FAILED(result = SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE)))
	//{
	//	buffer += L"Initialization of DPI failed. Exception from HRESULT: ";
	//	buffer += _com_error(result).ErrorMessage();
	//	buffer += '\n';
	//}

	if (FAILED(result = Initialize()))
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
	run.popup = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_POPUP_MENU)); // Load our popup menu

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
		cnst.program_name,						// lpszClassName
		NULL									// hIconSm
	};

	RegisterClassEx(&wnd);

	run.tray = {
		sizeof(run.tray),												// cbSize
		CreateWindowEx(													// hWnd
			WS_EX_TOOLWINDOW,													// dwExStyle
			cnst.program_name,													// lpClassName
			L"TrayWindow",														// lpWindowName
			WS_OVERLAPPEDWINDOW,												// dwStyle
			0,																	// x
			0,																	// y
			400,																// nWidth
			400,																// nHeight
			NULL,																// hWndParent
			NULL,																// hMenu
			hInstance,															// hInstance
			NULL																// lpParam
		),
		101,															// uID
		NIF_ICON | NIF_TIP | NIF_MESSAGE,								// uFlags
		cnst.WM_NOTIFY_TB,												// uCallbackMessage
		LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(MAINICON))		// hIcon
	};
	wcscpy_s(run.tray.szTip, cnst.program_name);

	ShowWindow(run.tray.hWnd, WM_SHOWWINDOW);
	RegisterTray();
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int)
{
	// If there already is another instance running, tell it to exit
	if (!IsSingleInstance()) {
		HWND oldInstance = FindWindow(cnst.program_name, L"TrayWindow");
		SendMessage(oldInstance, cnst.NEW_TTB_INSTANCE, NULL, NULL);
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

		MessageBox(NULL, message.c_str(), (std::wstring(cnst.program_name) + L" - Fatal error").c_str(), MB_ICONERROR | MB_OK);
		return 1;
	}

	// If the configuration files don't exist, restore the files and show welcome to the users
	CheckAndRunWelcome();

	// Verify our runtime
	run.fluent_available = IsAtLeastBuildNumber(17063);

	// Parse our configuration
	ParseConfigFile();
	ParseBlacklistFile();

	// Initialize GUI
	InitializeTray(hInstance);

	// Populate our vectors
	RefreshHandles();

	// Scan windows to start taskbar with the correct mode immediatly
	if (opt.dynamicws || opt.peek == Dynamic)
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
	if (run.exit_reason != NewInstance)
	{
		if (run.exit_reason != UserActionNoSave)
		{
			// Save configuration before we change opt
			SaveConfigFile();
		}

		// Restore default taskbar appearance
		opt.taskbar_appearance = ACCENT_NORMAL;
		opt.peek = Enabled;
		opt.dynamicstart = false;
		opt.dynamicws = false;
		SetTaskbarBlur();
	}

	// Close the uniqueness handle to allow other instances to run
	CloseHandle(run.ev);

	// Uninitialize UWP and COM
	CoUninitialize();
	Uninitialize();

	// Notify Explorer we are exiting
	Shell_NotifyIcon(NIM_DELETE, &run.tray);

	// Exit
	return 0;
}

#pragma endregion