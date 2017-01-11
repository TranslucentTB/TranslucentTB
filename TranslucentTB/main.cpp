#include <windows.h>
#include <iostream>
#include <string>

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
			exit(0);
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