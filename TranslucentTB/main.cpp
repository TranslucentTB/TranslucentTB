#include <windows.h>
//#include <iostream>

const HINSTANCE hModule = LoadLibraryW(L"user32.dll");

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

const int APPEARANCE_BLURRY = 1;
const int APPEARANCE_OPAQUE = 2;
const int APPEARANCE_TRANSPARENT = 3;

const int ACCENT_ENABLE_GRADIENT = 1;
const int ACCENT_ENABLE_TRANSPARENTGRADIENT = 2;
const int ACCENT_ENABLE_BLURBEHIND = 3;


typedef BOOL(WINAPI*pSetWindowCompositionAttribute)(HWND, WINCOMPATTRDATA*);
const pSetWindowCompositionAttribute SetWindowCompositionAttribute = (pSetWindowCompositionAttribute)GetProcAddress(hModule, "SetWindowCompositionAttribute");

void SetWindowBlur(HWND hWnd)
{
	if (hModule)
	{
		if (SetWindowCompositionAttribute)
		{
			ACCENTPOLICY policy;

			if (opt.taskbar_appearance == APPEARANCE_OPAQUE)
			{
				// Makes the taskbar a solid color specified by nColor.
				// This mode doesn't care about the alpha channel.
				// nFlags doesn't seem to matter.
				// nAccentState = ACCENT_ENABLE_GRADIENT = 1

				policy = { ACCENT_ENABLE_GRADIENT, 0, opt.color, 0 };
			}
			else if (opt.taskbar_appearance == APPEARANCE_TRANSPARENT)
			{
				// Makes the taskbar a tinted transparent overlay.
				// nColor is the tint color. A value of zero makes it opaque
				// and uses the system selected color scheme. Anything else
				// applies a tint to the taskbar, and the alpha channel
				// determines the opacity.
				// nFlags doesn't seem to matter.
				// nAccentState = ACCENT_ENABLE_TRANSPARENTGRADIENT = 2

				// This has been really inconsistent on my system, sometimes the 
				// taskbar just turns to a solid white, but launching the program
				// a few more times with different colors in between will eventually
				// apply the color tint as expected.

				policy = { ACCENT_ENABLE_TRANSPARENTGRADIENT, 0, opt.color, 0 };
			}
			else if (opt.taskbar_appearance == APPEARANCE_BLURRY)
			{
				// This mode only blurs the taskbar, the nColor
				// field has no effect.
				// nFlags doesn't matter
				// nAccentState = ACCENT_ENABLE_BLURBEHIND = 3

				policy = { ACCENT_ENABLE_BLURBEHIND, 0, opt.color, 0 };
			}

			WINCOMPATTRDATA data = { 19, &policy, sizeof(ACCENTPOLICY) }; // WCA_ACCENT_POLICY=19
			SetWindowCompositionAttribute(hWnd, &data);
		}

	}
}


void PrintHelp()
{
	/*
	The following won't print anything unless we explicitly call AllocConsole() and reopen stdout to
	point to the console, or change the project settings to target the console subsystem instead
	of the windows subsystem. Both have to problem of displaying an unwanted console in an
	otherwise silent program. I'm not sure what the best solution would be - the simplest might be
	to just write a small usage.txt file to accompany the program.
	*/

	/*
	using namespace std;
	wcout << "TranslucentTB by /u/IronManMark20" << endl;
	wcout << "This program modifies the apperance of the windows taskbar" << endl;
	wcout << "You can modify its behaviour by using the following parameters when launching the program:" << endl;
	wcout << "  --blur        | will make the taskbar a blurry overlay of the background (default)." << endl;
	wcout << "  --opaque      | will make the taskbar a solid color specified by the tint parameter." << endl;
	wcout << "  --transparent | will make the taskbar a transparent color specified by the tint parameter. " << endl;
	wcout << "                  the value of the alpha channel determines the opacity of the taskbar." << endl;
	wcout << "  --tint COLOR  | specifies the color applied to the taskbar. COLOR is a hexadecimal 32 bit number." << endl;
	wcout << "                  Will not affect the blur mode. By choosing a COLOR of zero in combination with --transparent" << endl;
	wcout << "                  the taskbar becomes opaque and uses the selected system color scheme." << endl;
	wcout << "  --help        | Displays this help message." << endl;
	*/
}

void ParseOptions()
{
	// Set default value
	opt.taskbar_appearance = APPEARANCE_BLURRY;

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
		}
		else if (wcscmp(arg, L"--blur") == 0)
		{
			opt.taskbar_appearance = APPEARANCE_BLURRY;
		}
		else if (wcscmp(arg, L"--opaque") == 0)
		{
			opt.taskbar_appearance = APPEARANCE_OPAQUE;
		}
		else if (wcscmp(arg, L"--transparent") == 0)
		{
			opt.taskbar_appearance = APPEARANCE_TRANSPARENT;
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


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreInst, LPSTR pCmdLine, int nCmdShow)
{
	ParseOptions();

	HWND taskbar = FindWindowW(L"Shell_TrayWnd", NULL);
	while (true)
	{
		SetWindowBlur(taskbar); Sleep((DWORD)10);
	}
	FreeLibrary(hModule);

}