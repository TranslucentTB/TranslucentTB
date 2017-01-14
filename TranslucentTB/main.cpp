#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <iterator>
#include <stdio.h>

//used for the tray things
#include <shellapi.h>
#include "resource.h"

//we use a GUID for uniqueness
const static LPCWSTR singleProcName = L"344635E9-9AE4-4E60-B128-D53E25AB70A7";

bool run = true; //needed for tray exit
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
	bool dynamic;
} opt;

enum TASKBARSTATE { Normal, WindowMaximised, StartMenuOpen }; // Create a state to store all 
															  // states of the Taskbar
			// Normal          | Proceed as normal. If no dynamic options are set, act as it says in opt.taskbar_appearance
			// WindowMaximised | There is a window which is maximised on the monitor this HWND is in. Display as blurred.
			// StartMenuOpen   | The Start Menu is open on the monitor this HWND is in. Display as it would be without TranslucentTB active.

struct TASKBARPROPERTIES
{
	HMONITOR hmon;
	TASKBARSTATE state;
};

int counter = 0;
const int ACCENT_ENABLE_GRADIENT = 1; // Makes the taskbar a solid color specified by nColor. This mode doesn't care about the alpha channel.
const int ACCENT_ENABLE_TRANSPARENTGRADIENT = 2; // Makes the taskbar a tinted transparent overlay. nColor is the tint color, sending nothing results in it interpreted as 0x00000000 (totally transparent, blends in with desktop)
const int ACCENT_ENABLE_BLURBEHIND = 3; // Makes the taskbar a tinted blurry overlay. nColor is same as above.
unsigned int WM_TASKBARCREATED;
BOOL _transparent;
std::map<HWND, TASKBARPROPERTIES> taskbars; // Create a map for all taskbars


typedef BOOL(WINAPI*pSetWindowCompositionAttribute)(HWND, WINCOMPATTRDATA*);
static pSetWindowCompositionAttribute SetWindowCompositionAttribute = (pSetWindowCompositionAttribute)GetProcAddress(GetModuleHandle(TEXT("user32.dll")), "SetWindowCompositionAttribute");

void SetWindowBlur(HWND hWnd, int appearance = 0) // `appearance` can be 0, which means 'follow opt.taskbar_appearance'
{
	if (SetWindowCompositionAttribute)
	{
		ACCENTPOLICY policy;

		if (appearance) // Custom taskbar appearance is set
		{
			policy = { appearance, 2, opt.color, 0 };
		} else { // Use the defaults
			policy = { opt.taskbar_appearance, 2, opt.color, 0 };
		}
		

		WINCOMPATTRDATA data = { 19, &policy, sizeof(ACCENTPOLICY) }; // WCA_ACCENT_POLICY=19
		SetWindowCompositionAttribute(hWnd, &data);
	}

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
			cout << "TranslucentTB by /u/IronManMark20 (and others)" << endl;
			cout << "This program modifies the apperance of the Windows 10 taskbar" << endl;
			cout << "You can modify its behaviour by using the following parameters when launching the program:" << endl;
			cout << "  --blur        | will make the taskbar a blurry overlay of the background (default)." << endl;
			cout << "  --opaque      | will make the taskbar a solid color specified by the tint parameter." << endl;
			cout << "  --transparent | will make the taskbar a transparent color specified by the tint parameter. " << endl;
			cout << "                  the value of the alpha channel determines the opacity of the taskbar." << endl;
			cout << "  --tint COLOR  | specifies the color applied to the taskbar. COLOR is 32 bit number in hex format," << endl;
			cout << "                  see explanation below. This will not affect the blur mode. If COLOR is zero in" << endl;
			cout << "                  combination with --transparent the taskbar becomes opaque and uses the selected" << endl;
			cout << "                  system color scheme." << endl;
			cout << "  --dynamic     | will make the taskbar transparent when no windows are maximised in the current" << endl;
			cout << "                  monitor, otherwise blurry." << endl;
			cout << "  --help        | display this help message." << endl;
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

void ParseOptions()
{
	// Set default values
	opt.taskbar_appearance = ACCENT_ENABLE_BLURBEHIND;
	opt.color = 0x00000000;

	// Loop through command line arguments
	LPWSTR *szArglist;
	int nArgs;

	szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);

	for (int i = 0; i < nArgs; i++)
	{
		LPWSTR arg = szArglist[i];

		if (wcscmp(arg, L"--help") == 0)
		{
			PrintHelp();
			exit(0);
		}
		else if (wcscmp(arg, L"--blur") == 0)
		{
			opt.taskbar_appearance = ACCENT_ENABLE_BLURBEHIND;
		}
		else if (wcscmp(arg, L"--opaque") == 0)
		{
			opt.taskbar_appearance = ACCENT_ENABLE_GRADIENT;
		}
		else if (wcscmp(arg, L"--transparent") == 0)
		{
			opt.taskbar_appearance = ACCENT_ENABLE_TRANSPARENTGRADIENT;
		}
		else if (wcscmp(arg, L"--dynamic") == 0)
		{
			opt.taskbar_appearance = ACCENT_ENABLE_TRANSPARENTGRADIENT;
			opt.dynamic = true;
		}
		else if (wcscmp(arg, L"--tint") == 0)
		{
			// The next argument should be a color in hex format
			if (i + 1 < nArgs && wcslen(szArglist[i + 1]) > 0)
			{
				LPWSTR param = szArglist[i + 1];
				unsigned long colval = 0;
				WCHAR* stopchar;


				colval = wcstoul(param, &stopchar, 16);


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

	LocalFree(szArglist);
}

#pragma endregion

#pragma region tray

#define WM_NOTIFY_TB 3141

HMENU menu;

void RefreshHandles()
{
	HWND _taskbar;
	HMONITOR _monitor;
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
				break;
			case IDM_CLEAR:
				CheckMenuRadioItem(popup, IDM_BLUR, IDM_CLEAR, IDM_CLEAR, MF_BYCOMMAND);
				opt.taskbar_appearance = ACCENT_ENABLE_TRANSPARENTGRADIENT;
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

BOOL CALLBACK EnumWindowsProcess(HWND hWnd, LPARAM lParam) 
{
	HMONITOR _monitor;

	WINDOWPLACEMENT result = {};
	::GetWindowPlacement(hWnd, &result);
	if(result.showCmd == 3) { 
		_monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTOPRIMARY);
		for (auto &taskbar: taskbars)
		{
			if (taskbar.second.hmon == _monitor)
			{
				taskbar.second.state = WindowMaximised;
			}
		}
	}

	return true;
}

void SetTaskbarBlur()
{
	if (opt.dynamic) {
		if (counter >= 5)   // Change this if you want to change the time it takes for the program to update
		{                   // 100 = 1 second; we use 5, because the difference is less noticeable and it has
							// no large impact on CPU. We can change this if we feel that CPU is more important
							// than response time.
			counter = 0;
			for (auto &taskbar: taskbars)
			{
				taskbar.second.state = Normal; // Reset taskbar state
			}
			EnumWindows(&EnumWindowsProcess, NULL);
		}
	}
	
	for (auto const &taskbar: taskbars)
	{
		if (taskbar.second.state == WindowMaximised)
		{
			SetWindowBlur(taskbar.first, ACCENT_ENABLE_BLURBEHIND);
											// A window is maximised; let's make sure that we blur the window.
		} else if (taskbar.second.state == Normal) {
			SetWindowBlur(taskbar.first);  // Taskbar should be normal, call using normal transparency settings
		}
	} 
	counter++;
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
	WINDOWPLACEMENT result;
	result.length = sizeof(WINDOWPLACEMENT); // For temporary variable
	if (singleProc()) {
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
		ParseOptions(); //command line argument settings
		if (opt.taskbar_appearance == ACCENT_ENABLE_BLURBEHIND)
		{
			CheckMenuRadioItem(popup, IDM_BLUR, IDM_CLEAR, IDM_BLUR, MF_BYCOMMAND);
		}
		else if (opt.taskbar_appearance == ACCENT_ENABLE_TRANSPARENTGRADIENT)
		{
			CheckMenuRadioItem(popup, IDM_BLUR, IDM_CLEAR, IDM_CLEAR, MF_BYCOMMAND);
		}

		RefreshHandles();
		if (opt.dynamic)
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
	}
	CloseHandle(ev);
	return 0;
}