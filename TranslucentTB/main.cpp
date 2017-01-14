#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>

//used for the tray things
#include <shellapi.h>
#include "resource.h"

//we use a GUID for uniqueness
const static LPCWSTR singleProcName = L"344635E9-9AE4-4E60-B128-D53E25AB70A7";

//needed for tray exit
bool run = true; 

// config file path (defaults to ./config.cfg)
std::wstring configfile;
// holds whether the user passed a --config parameter on the command line
bool explicitconfig;
bool shouldsaveconfig;

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
} opt;

const int ACCENT_ENABLE_GRADIENT = 1; // Makes the taskbar a solid color specified by nColor. This mode doesn't care about the alpha channel.
const int ACCENT_ENABLE_TRANSPARENTGRADIENT = 2; // Makes the taskbar a tinted transparent overlay. nColor is the tint color, sending nothing results in it interpreted as 0x00000000 (totally transparent, blends in with desktop)
const int ACCENT_ENABLE_BLURBEHIND = 3; // Makes the taskbar a tinted blurry overlay. nColor is same as above.
unsigned int WM_TASKBARCREATED;



typedef BOOL(WINAPI*pSetWindowCompositionAttribute)(HWND, WINCOMPATTRDATA*);
static pSetWindowCompositionAttribute SetWindowCompositionAttribute = (pSetWindowCompositionAttribute)GetProcAddress(GetModuleHandle(TEXT("user32.dll")), "SetWindowCompositionAttribute");

void SetWindowBlur(HWND hWnd)
{
	if (SetWindowCompositionAttribute)
	{
		ACCENTPOLICY policy;

		policy = { opt.taskbar_appearance, 2, opt.color, 0 };

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
			cout << "TranslucentTB by /u/IronManMark20" << endl;
			cout << "This program modifies the apperance of the windows taskbar" << endl;
			cout << "You can modify its behaviour by using the following parameters when launching the program:" << endl;
			cout << "  --blur        | will make the taskbar a blurry overlay of the background (default)." << endl;
			cout << "  --opaque      | will make the taskbar a solid color specified by the tint parameter." << endl;
			cout << "  --transparent | will make the taskbar a transparent color specified by the tint parameter. " << endl;
			cout << "                  the value of the alpha channel determines the opacity of the taskbar." << endl;
			cout << "  --tint COLOR  | specifies the color applied to the taskbar. COLOR is 32 bit number in hex format," << endl;
			cout << "                  see explanation below. This will not affect the blur mode. If COLOR is zero in" << endl;
			cout << "                  combination with --transparent the taskbar becomes opaque and uses the selected" << endl;
			cout << "                  system color scheme." << endl;
			cout << "  --help        | Displays this help message." << endl;
			cout << endl;

			cout << "Color format:" << endl;
			cout << "  The parameter is interpreted as a three or four byte long number in hexadecimal format that" << endl;
			cout << "  describes the four color channels ([alpha,] red, green and blue). These look like this:" << endl;
			cout << "  0x80fe10a4 (the '0x' is optional). You often find colors in this format in the context of HTML and" << endl;
			cout << "  web design, and there are many online tools to convert from familiar names to this format. These" << endl;
			cout << "  tools might give you numbers starting with '#', in that case you can just remove the leading '#'." << endl;
			cout << "  You should be able to find online tools by searching for \"color to hex\" or something similar." << endl;
			cout << "  If the converter doesn't include alpha values (opacity), you can append them yourself at the start" << endl;
			cout << "  of the number. Just convert a value between 0 and 255 to its hexadecimal value before you append it." << endl;
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
		configstream << L"; Color and opacity of the taskbar." << endl;

		// TODO include the alpha channel here or not?
		unsigned int bitreversed =
			(opt.color & 0xFF000000) +
			((opt.color & 0x00FF0000) >> 16) +
			(opt.color & 0x0000FF00) +
			((opt.color & 0x000000FF) << 16);
		configstream << L"color=" << hex << bitreversed << L"    ; A color in hexadecimal notation. Described in usage.md." << endl;
		configstream << L"opacity=" << to_wstring((opt.color & 0xFF000000) >> 24) << L"    ; A value in the range 0 to 255." << endl;
	}
}

void ParseSingleOption(std::wstring arg, std::wstring value)
{
	if (arg == L"--help")
	{
		PrintHelp();
		exit(0);
	}
	else if (arg == L"--config")
	{
		// Ignore - this was handled in a previous iteration
		// over the arguments.
	}
	else if (arg == L"--blur")
	{
		opt.taskbar_appearance = ACCENT_ENABLE_BLURBEHIND;
	}
	else if (arg == L"--opaque")
	{
		opt.taskbar_appearance = ACCENT_ENABLE_GRADIENT;
	}
	else if (arg == L"--transparent")
	{
		opt.taskbar_appearance = ACCENT_ENABLE_TRANSPARENTGRADIENT;
	}
	else if (arg == L"--tint")
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
}

void ParseCmdOptions()
{
	// Set default values
	shouldsaveconfig = false;
	explicitconfig = false;
	configfile = L"config.cfg";
	forcedtransparency = -1;

	opt.taskbar_appearance = ACCENT_ENABLE_BLURBEHIND;
	opt.color = 0x00000000;

	// Loop through command line arguments
	LPWSTR *szArglist;
	int nArgs;

	szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);


	// Find the --config option if provided
	for (int i = 0; i < nArgs; i++)
	{
		LPWSTR lparg = szArglist[i];
		LPWSTR lpvalue = (i + 1 < nArgs) ? szArglist[i + 1] : L"";

		std::wstring arg = std::wstring(lparg);
		std::wstring value = std::wstring(lpvalue);

		if (arg == L"--config")
		{
			// We allow multiple --config options. The later ones will override the previous ones.
			// The lates will be assigned to configfile, and that's where changes are saved.
			if (value.length() > 0 &&
				file_exists(value))
			{
				configfile = value;
				ParseConfigFile(value);
			}
			// TODO else? Missing or invalid parameter, should log
		}
	}

	// Iterate over the rest of the arguments 
	// Those options override the config files.
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
	taskbar = FindWindowW(L"Shell_TrayWnd", NULL);
	secondtaskbar = FindWindow(L"Shell_SecondaryTrayWnd", NULL); // we use this for the taskbars on other monitors.
}

#pragma endregion

#pragma region tray

#define WM_NOTIFY_TB 3141

HMENU menu;

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
			switch (tray)
			{
			case IDM_BLUR:
				CheckMenuRadioItem(popup, IDM_BLUR, IDM_CLEAR, IDM_BLUR, MF_BYCOMMAND);
				opt.taskbar_appearance = ACCENT_ENABLE_BLURBEHIND;
				shouldsaveconfig = true;
				break;
			case IDM_CLEAR:
				CheckMenuRadioItem(popup, IDM_BLUR, IDM_CLEAR, IDM_CLEAR, MF_BYCOMMAND);
				opt.taskbar_appearance = ACCENT_ENABLE_TRANSPARENTGRADIENT;
				shouldsaveconfig = true;
				break;
			case IDM_EXIT:
				run = false;
				break;
			}
		}
	}
	if (message == WM_TASKBARCREATED) // Unfortunately, WM_TASKBARCREATED is not a constant, so I can't include it in the switch.
	{
		RefreshHandles();
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

NOTIFYICONDATA Tray;

void initTray(HWND parent)
{

	Tray.cbSize = sizeof(Tray);
	Tray.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(MAINICON));
	Tray.hWnd = parent;
	wcscpy_s(Tray.szTip, L"TransparentTB");
	Tray.uCallbackMessage = WM_NOTIFY_TB;
	Tray.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
	Tray.uID = 101;
	Shell_NotifyIcon(NIM_ADD, &Tray);
	Shell_NotifyIcon(NIM_SETVERSION, &Tray);
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
	if (singleProc()) {
		// TODO Usually the command line overrides the config file, so these should be reversed.
		// Still, we check the --config parameter on the command line, so that should be handled
		// before the config file. Needs two passes on the command line arguments, lets leave that
		// for later.
		ParseCmdOptions(); //command line argument settings
		ParseConfigFile(); //config file settings

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

		HWND tray_hwnd = CreateWindowEx(WS_EX_TOOLWINDOW, L"TranslucentTB", L"TrayWindow", WS_OVERLAPPEDWINDOW, 0, 0,
			400, 400, NULL, NULL, hInstance, NULL);

		initTray(tray_hwnd);

		ShowWindow(tray_hwnd, WM_SHOWWINDOW);
		if (opt.taskbar_appearance == ACCENT_ENABLE_BLURBEHIND)
		{
			CheckMenuRadioItem(popup, IDM_BLUR, IDM_CLEAR, IDM_BLUR, MF_BYCOMMAND);
		}
		else if (opt.taskbar_appearance == ACCENT_ENABLE_TRANSPARENTGRADIENT)
		{
			CheckMenuRadioItem(popup, IDM_BLUR, IDM_CLEAR, IDM_CLEAR, MF_BYCOMMAND);
		}

		RefreshHandles();
		WM_TASKBARCREATED = RegisterWindowMessage(L"TaskbarCreated");
		while (run) {
			SetWindowBlur(taskbar);
			if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			while (secondtaskbar = FindWindowEx(0, secondtaskbar, L"Shell_SecondaryTrayWnd", L""))
			{
				SetWindowBlur(secondtaskbar);
			}
			Sleep(10);
		}
		Shell_NotifyIcon(NIM_DELETE, &Tray);

		if (shouldsaveconfig)
			SaveConfigFile();
	}
	CloseHandle(ev);
	return 0;
}