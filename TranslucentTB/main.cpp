#include <windows.h>
#include <fstream>
#include <iostream>

using namespace std;

const HINSTANCE hModule = LoadLibrary(TEXT("user32.dll"));

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



typedef BOOL(WINAPI*pSetWindowCompositionAttribute)(HWND, WINCOMPATTRDATA*);
const pSetWindowCompositionAttribute SetWindowCompositionAttribute = (pSetWindowCompositionAttribute)GetProcAddress(hModule, "SetWindowCompositionAttribute");

void SetWindowBlur(HWND hWnd)
{
	
	if (hModule)
	{	
		if (SetWindowCompositionAttribute)
		{
			ACCENTPOLICY policy;
			ifstream file("transparent.xml");
			if(!file)
				policy = { 3, 0, 0, 0 }; // ACCENT_ENABLE_BLURBEHIND=3, ACCENT_INVALID=4...
			else
				policy = {2, 2, 0, 0};
			WINCOMPATTRDATA data = { 19, &policy, sizeof(ACCENTPOLICY) }; // WCA_ACCENT_POLICY=19
			SetWindowCompositionAttribute(hWnd, &data);
		}
		
	}
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreInst, LPSTR pCmdLine, int nCmdShow)
{

	HWND otherTb ;
	
	while (true) {
		SetWindowBlur(FindWindowA("Shell_TrayWnd", NULL)); 
		while (otherTb = FindWindowEx(0, otherTb, "Shell_SecondaryTrayWnd", ""))
    		SetWindowBlur(otherTb);
		Sleep((DWORD)10);
	}
	FreeLibrary(hModule);

}
