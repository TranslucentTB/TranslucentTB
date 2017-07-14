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

#include <algorithm>

// for making the menu show up better
#include <ShellScalingAPI.h>

//used for the tray things
#include <shellapi.h>
#include "resource.h"

//we use a GUID for uniqueness
const static LPCWSTR singleProcName = L"344635E9-9AE4-4E60-B128-D53E25AB70A7";

//needed for tray exit
bool run = true;
bool hastray = true;

// config file path (defaults to ./config.cfg)
std::wstring configfile;
// holds whether the user passed a --config parameter on the command line
bool explicitconfig;

// holds the alpha channel value between 0 or 255,
// defaults to -1 (not set).
int forcedtransparency;

HWND taskbar;
HWND secondtaskbar;
HMENU popup;

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

struct OPTIONS
{
	int taskbar_appearance;
	int color;
	bool dynamicws;
	bool dynamicstart;
} opt;

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
int DYNAMIC_WS_STATE = ACCENT_ENABLE_BLURBEHIND; // State to activate when d-ws is enabled
std::map<HWND, TASKBARPROPERTIES> taskbars; // Create a map for all taskbars

IVirtualDesktopManager *desktop_manager;

typedef BOOL(WINAPI*pSetWindowCompositionAttribute)(HWND, WINCOMPATTRDATA*);
static pSetWindowCompositionAttribute SetWindowCompositionAttribute = (pSetWindowCompositionAttribute)GetProcAddress(GetModuleHandle(TEXT("user32.dll")), "SetWindowCompositionAttribute");

void SetWindowBlur(HWND hWnd, int appearance = 0) // `appearance` can be 0, which means 'follow opt.taskbar_appearance'
{
	if (SetWindowCompositionAttribute)
	{
		ACCENTPOLICY policy;

		if (appearance) // Custom taskbar appearance is set
		{
			if (DYNAMIC_WS_STATE == ACCENT_ENABLE_TINTED)
			{ // dynamic-ws is set to tint
				if (appearance == ACCENT_ENABLE_TINTED) { policy = { ACCENT_ENABLE_TRANSPARENTGRADIENT, 2, opt.color, 0 }; } // Window is maximised
				else { policy = { ACCENT_ENABLE_TRANSPARENTGRADIENT, 2, 0x00000000, 0 }; } // Desktop is shown (this shouldn't ever be called tho, just in case)
			}
			else { policy = { appearance, 2, opt.color, 0 }; }
		}
		else { // Use the defaults
			if (DYNAMIC_WS_STATE == ACCENT_ENABLE_TINTED) { policy = { ACCENT_ENABLE_TRANSPARENTGRADIENT, 2, 0x00000000, 0 }; } // dynamic-ws is tint and desktop is shown
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

#pragma region command line
void PrintHelp()
{
	// BUG - 
	// For some reason, when launching this program in cmd.exe, it won't properly "notice"
	// when the program has exited, and will not automatically write a new prompt to the console.
	// Instead of printing the current directory as usual before it waits for a new command,
	// it doesn't print anything, leading to a cluttered console.
	// It's even worse in Powershell, where it actually WILL print the PS prompt before waiting for 
	// a new command, but it does so on the line after "./TranslucentTB.exe --help", overwriting the 
	// first line of output from this function, and gradually overwriting the following lines as you
	// press enter. The PS shell just doesn't notice that anything gets printed to the console, and
	// therefore it prints the PS prompt over this output instead of after. I don't know of any 
	// solution to this, but I expect that setting the project type to SUBSYSTEM:CONSOLE would solve
	// those issues. Again - I think a help file would be the best solution, so I'll do that in my 
	// next commit.

	BOOL hasconsole = true;
	BOOL createdconsole = false;
	// Try to attach to the parent console,
	// allocate a new one if that isn't successful
	if (!AttachConsole(ATTACH_PARENT_PROCESS))
	{
		if (!AllocConsole())
		{
			hasconsole = false;
		}
		else
		{
			createdconsole = true;
		}
	}

	if (hasconsole)
	{
		FILE* outstream;
		FILE* instream;
		freopen_s(&outstream, "CONOUT$", "w", stdout);
		freopen_s(&instream, "CONIN$", "w", stdin);

		if (outstream)
		{
			using namespace std;
			cout << endl;
			cout << "TranslucentTB by ethanhs & friends" << endl;
			cout << "Source: https://github.com/TranslucentTB/TranslucentTB" << endl;
			cout << "    TranslucentTB is freeware and is distributed under thte open-source GPL3 license." << endl;
			cout << "This program modifies the appearance of the windows taskbar" << endl;
			cout << "You can modify its behaviour by using the following parameters when launching the program:" << endl;
			cout << "  --blur                | will make the taskbar a blurry overlay of the background (default)." << endl;
			cout << "  --opaque              | will make the taskbar a solid color specified by the tint parameter." << endl;
			cout << "  --transparent         | will make the taskbar a transparent color specified by the tint parameter." << endl;
			cout << "                          The value of the alpha channel determines the opacity of the taskbar." << endl;
			cout << "  --tint COLOR          | specifies the color applied to the taskbar. COLOR is 32 bit number in hex format," << endl;
			cout << "                          see explanation below." << endl;
			cout << "  --dynamic-ws STATE    | will make the taskbar transparent when no windows are maximised in the current" << endl;
			cout << "                          monitor, otherwise blurry. State can be from: (blur, opaque, tint). Blur is default." << endl;
			cout << "  --dynamic-start       | will make the taskbar return to it's normal state when the start menu is opened," << endl;
			cout << "                          current setting otherwise." << endl;
			cout << "  --help                | Displays this help message." << endl;
			cout << "  --no-tray             | will hide the taskbar tray icon." << endl;
			cout << "  --with-tray           | Ignore no-tray in config file." << endl;
			cout << endl;

			cout << "Color format:" << endl;
			cout << "  The parameter is interpreted as a three or four byte long number in hexadecimal format that" << endl;
			cout << "  describes the four color channels 0xAARRGGBB ([alpha,] red, green and blue). These look like this:" << endl;
			cout << "  0x80fe10a4 (the '0x' is optional). You often find colors in this format in the context of HTML and" << endl;
			cout << "  web design, and there are many online tools to convert from familiar names to this format. These" << endl;
			cout << "  tools might give you numbers starting with '#', in that case you can just remove the leading '#'." << endl;
			cout << "  You should be able to find online tools by searching for \"color to hex\" or something similar." << endl;
			cout << "  If the converter doesn't include alpha values (opacity), you can append them yourself at the start" << endl;
			cout << "  of the number. Just convert a value between 0 and 255 to its hexadecimal value before you append it." << endl;
			cout << endl;
			cout << "Examples:" << endl;
			cout << "# start with Windows, start transparent" << endl;
			cout << "TranslucentTB.exe --startup --transparent --save-all" << endl;
			cout << "# run dynamic windows mode, with the supplied color" << endl;
			cout << "TranslucentTB.exe --tint 80fe10a4 --dynamic-ws tint" << endl;
			cout << "# Will be normal when start is open, transparent otherwise." << endl;
			cout << "TranslucentTB.exe --dynamic-start" << endl;
			cout << endl;
			cout << "Config files and startup settings for Windows Store edition" << endl;
			cout << "The config and dynamic exclude files are located under %LocalAppData%\\TranslucentTB and will always be loaded" << endl;
			cout << "when the program starts and the config file will be saved on exit. --config, --exclude-file --save-all" << endl;
			cout << "have been disabled." << endl;
			cout << endl;
			cout << "Startup settings for Windows Store Edition" << endl;
			cout << "Centennial applications must use a special API for registering themselves at startup. The user can" << endl;
			cout << "manage the startup state from either task manager or the tray icon. If disabled from task manager, the" << endl;
			cout << "option in the tray icon will be greyed out." << endl;
			cout << endl;

			if (createdconsole && instream)
			{
				string wait;

				cout << "Press enter to exit the program." << endl;
				if (!getline(cin, wait))
				{
					// Couldn't wait for user input, make the user close
					// the program themselves so they can see the output.
					cout << "Press Ctrl + C, Alt + F4, or click the close button to exit the program." << endl;
					Sleep(INFINITE);
				}

				FreeConsole();
			}

			fclose(outstream);
		}
	}
}

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
			opt.taskbar_appearance = ACCENT_ENABLE_TRANSPARENTGRADIENT;
			opt.dynamicws = true;
		}
		else if (value == L"tint")
		{
			opt.taskbar_appearance = ACCENT_ENABLE_TRANSPARENTGRADIENT;
			opt.dynamicws = true;
			DYNAMIC_WS_STATE = ACCENT_ENABLE_TINTED;
		}
		else if (value == L"blur")
		{
			opt.taskbar_appearance = ACCENT_ENABLE_TRANSPARENTGRADIENT;
			opt.dynamicws = true;
			DYNAMIC_WS_STATE = ACCENT_ENABLE_BLURBEHIND;
		}
		else if (value == L"opaque")
		{
			opt.taskbar_appearance = ACCENT_ENABLE_TRANSPARENTGRADIENT;
			opt.dynamicws = true;
			DYNAMIC_WS_STATE = ACCENT_ENABLE_GRADIENT;
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
	else if (arg == L"no-tray")
	{
		if (value == L"true" ||
			value == L"enable")
		{
			hastray = false;
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

void SaveConfigFile()
{
	if (!configfile.empty())
	{
		using namespace std;
		wofstream configstream(configfile);

		configstream << L"; Taskbar appearance: opaque, transparent, or blur (default)." << endl;

		if (opt.taskbar_appearance == ACCENT_ENABLE_GRADIENT)
			configstream << L"accent=opaque" << endl;
		else if (opt.taskbar_appearance == ACCENT_ENABLE_TRANSPARENTGRADIENT)
			configstream << L"accent=transparent" << endl;
		else if (opt.taskbar_appearance == ACCENT_ENABLE_BLURBEHIND)
			configstream << L"accent=blur" << endl;

		configstream << endl;
		configstream << L"; Dynamic states: Window States and (WIP) Start Menu" << endl;
		configstream << L"; dynamic windows: opaque, transparent, or blur (default)." << endl;
		if (!opt.dynamicws)
			configstream << L"; ";

		if (DYNAMIC_WS_STATE == ACCENT_ENABLE_BLURBEHIND)
			configstream << L"dynamic-ws=blur" << endl;
		else if (DYNAMIC_WS_STATE == ACCENT_ENABLE_TRANSPARENTGRADIENT)
			configstream << L"dynamic-ws=transparent" << endl;
		else if (DYNAMIC_WS_STATE == ACCENT_ENABLE_GRADIENT)
			configstream << L"dynamic-ws=opaque" << endl;
		else
			configstream << L"dynamic-ws=enable" << endl;

		configstream << L"; dynamic windows can be used in conjunction with a custom color and non-zero opacity!" << endl;

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
		if (hastray)
			configstream << L"; no-tray=enable" << endl;
		else
			configstream << L"no-tray=enable" << endl;
	}
}

void ParseSingleOption(std::wstring arg, std::wstring value)
{
	if (arg == L"--help")
	{
		PrintHelp();
		exit(0);
	}
	else if (arg == L"--blur")
	{
		opt.taskbar_appearance = ACCENT_ENABLE_BLURBEHIND;
	}
	else if (arg == L"--opaque")
	{
		opt.taskbar_appearance = ACCENT_ENABLE_GRADIENT;
	}
	else if (arg == L"--transparent" || arg == L"--clear")
	{
		opt.taskbar_appearance = ACCENT_ENABLE_TRANSPARENTGRADIENT;
	}
	else if (arg == L"--dynamic-ws")
	{
		opt.taskbar_appearance = ACCENT_ENABLE_TRANSPARENTGRADIENT;
		opt.dynamicws = true;
		if (value == L"tint") { DYNAMIC_WS_STATE = ACCENT_ENABLE_TINTED; }
		else if (value == L"blur") { DYNAMIC_WS_STATE = ACCENT_ENABLE_BLURBEHIND; }
		else if (value == L"opaque") { DYNAMIC_WS_STATE = ACCENT_ENABLE_GRADIENT; }
	}
	else if (arg == L"--dynamic-start")
	{
		opt.dynamicstart = true;
	}
	else if (arg == L"--tint" || arg == L"--color")
	{
		// The next argument should be a color in hex format
		if (value.length() > 0)
		{
			unsigned long colval = 0;
			size_t numchars;

			colval = stoul(value, &numchars, 16);

			// ACCENTPOLICY.nColor expects the byte order to be ABGR,
			// fiddle some bits to make it intuitive for the user.
			opt.color =
				(colval & 0xFF000000) +
				((colval & 0x00FF0000) >> 16) +
				(colval & 0x0000FF00) +
				((colval & 0x000000FF) << 16);
		}
		else
		{
			// TODO error handling for missing value
			// Really not much to do as we don't have functional
			// output streams, and opening a window seems overkill.
		}
	}
	else if (arg == L"--no-tray")
	{
		hastray = false;
	}
	else if (arg == L"--with-tray")
	{
		hastray = true;
	}
}

void ParseCmdOptions()
{
	// Loop through command line arguments
	LPWSTR *szArglist;
	int nArgs;

	szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);

	for (int i = 0; i < nArgs; i++)
	{
		LPWSTR lparg = szArglist[i];
		LPWSTR lpvalue = (i + 1 < nArgs) ? szArglist[i + 1] : L"";

		std::wstring arg = std::wstring(lparg);
		std::wstring value = std::wstring(lpvalue);

		ParseSingleOption(arg, value);
	}

	LocalFree(szArglist);
}

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

#pragma region tray

#define WM_NOTIFY_TB 3141

HMENU menu;
NOTIFYICONDATA Tray;
HWND tray_hwnd;

void RefreshMenu()
{
	if (opt.dynamicws)
	{
		CheckMenuRadioItem(popup, IDM_BLUR, IDM_DYNAMICWS, IDM_DYNAMICWS, MF_BYCOMMAND);
	}
	else if (opt.taskbar_appearance == ACCENT_ENABLE_BLURBEHIND)
	{
		CheckMenuRadioItem(popup, IDM_BLUR, IDM_DYNAMICWS, IDM_BLUR, MF_BYCOMMAND);
	}
	else if (opt.taskbar_appearance == ACCENT_ENABLE_TRANSPARENTGRADIENT)
	{
		CheckMenuRadioItem(popup, IDM_BLUR, IDM_DYNAMICWS, IDM_CLEAR, MF_BYCOMMAND);
	}
	else if (opt.taskbar_appearance == ACCENT_NORMAL_GRADIENT)
	{
		CheckMenuRadioItem(popup, IDM_BLUR, IDM_DYNAMICWS, IDM_NORMAL, MF_BYCOMMAND);
	}

	if (opt.dynamicstart)
	{
		CheckMenuItem(popup, IDM_DYNAMICSTART, MF_BYCOMMAND | MF_CHECKED);
	}
	else
	{
		CheckMenuItem(popup, IDM_DYNAMICSTART, MF_BYCOMMAND | MF_UNCHECKED);
	}

	if (RegGetValue(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", L"TranslucentTB", RRF_RT_REG_SZ, NULL, NULL, NULL) == ERROR_SUCCESS)
	{
		CheckMenuItem(popup, IDM_AUTOSTART, MF_BYCOMMAND | MF_CHECKED);
	}
	else {
		CheckMenuItem(popup, IDM_AUTOSTART, MF_BYCOMMAND | MF_UNCHECKED);
	}
}

void initTray(HWND parent)
{
	if (hastray)
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
}

BOOL isBlacklisted(HWND hWnd)
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

BOOL CALLBACK EnumWindowsProcess(HWND hWnd, LPARAM lParam)
{
	HMONITOR _monitor;

	if (opt.dynamicws)
	{
		WINDOWPLACEMENT result = {};
		::GetWindowPlacement(hWnd, &result);
		if (result.showCmd == SW_MAXIMIZE) {
			BOOL on_current_desktop;
			desktop_manager->IsWindowOnCurrentVirtualDesktop(hWnd, &on_current_desktop);
			if (IsWindowVisible(hWnd) && on_current_desktop)
			{
				if (!isBlacklisted(hWnd))
				{
					_monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTOPRIMARY);
					for (auto &taskbar : taskbars)
					{
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
			switch (tray) // TODO: Add menu items for colors, and one to open config file locations
			{
			case IDM_BLUR:
				opt.dynamicws = false;
				opt.taskbar_appearance = ACCENT_ENABLE_BLURBEHIND;
				break;
			case IDM_CLEAR:
				opt.dynamicws = false;
				opt.taskbar_appearance = ACCENT_ENABLE_TRANSPARENTGRADIENT;
				break;
			case IDM_NORMAL:
				opt.dynamicws = false;
				opt.taskbar_appearance = ACCENT_NORMAL_GRADIENT;
				break;
			case IDM_DYNAMICWS:
				opt.taskbar_appearance = ACCENT_ENABLE_TRANSPARENTGRADIENT;
				opt.dynamicws = true;
				EnumWindows(&EnumWindowsProcess, NULL);
				break;
			case IDM_DYNAMICSTART:
				opt.dynamicstart = !opt.dynamicstart;
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
		for (auto &taskbar : taskbars)
		{
			taskbar.second.state = Normal; // Reset taskbar state
		}
		if (opt.dynamicws) {
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
			SetWindowBlur(taskbar.first, DYNAMIC_WS_STATE);
			// A window is maximised; let's make sure that we blur the window.
		}
		else if (taskbar.second.state == Normal) {
			SetWindowBlur(taskbar.first);  // Taskbar should be normal, call using normal transparency settings
		}

	}
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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreInst, LPSTR pCmdLine, int nCmdShow)
{
	if (FAILED(SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE))) { OutputDebugStringW(L"Per-monitor DPI scaling failed\n"); }

	// Find user's localappdata folder
	// If !%localappdata%\TranslucentTB exists
	//     MessageBox(NULL, L"test", L"test", MB_ICONQUESTION | MB_OK);
	//
	// if !configfile exists
	//     copy from appx
	//
	// if !exclude file exists
	//     copy from appx
	//
	// configfile = %LocalAppData%\TranslucentTB\config.cfg
	ParseConfigFile(configfile); // Config file settings
	ParseCmdOptions(); // Command line argument settings
	// ParseDWSExcludesFile(L"%LocalAppData%\TranslucentTB\dynamic-ws-exclude.csv");

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

	//Virtual Desktop stuff
	if (FAILED(::CoInitialize(NULL))) { OutputDebugStringW(L"Initialization of COM failed, VirtualDesktopManager will probably fail too.\n"); }
	HRESULT desktop_success = ::CoCreateInstance(__uuidof(VirtualDesktopManager), NULL, CLSCTX_INPROC_SERVER, IID_IVirtualDesktopManager, (void **)&desktop_manager);
	if (FAILED(desktop_success)) { OutputDebugStringW(L"Initialization of VirtualDesktopManager failed\n"); }

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
		SaveConfigFile();

	opt.taskbar_appearance = ACCENT_NORMAL_GRADIENT;
	SetTaskbarBlur();
	CloseHandle(ev);
	return 0;
}
