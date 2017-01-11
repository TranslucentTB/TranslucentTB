#include <windows.h>
#include <fstream>
#include <stdio.h>
#include <iostream>
#include <vector>

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


void SetWindowBlur(HWND hWnd, BOOL transparent)
{
	
	if (hModule)
	{	
		if (SetWindowCompositionAttribute)
		{
			ACCENTPOLICY policy;

			if(!transparent)
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

	HWND mainTb, otherTb, fgWin;
	BOOL transparent, tpWhenMax;
	char str[256];
	WINDOWPLACEMENT result = {};
	result.length = sizeof(WINDOWPLACEMENT);
	
	std::vector<HWND> secondTbVec;

	// sprintf_s(str, "Item: %d !", secondTbVec[i]);
	// OutputDebugString(str);

	
	fgWin = GetForegroundWindow();
	::GetWindowPlacement(fgWin, &result);
	sprintf_s(str, "State: %d !", result.showCmd);
	OutputDebugString(str);

	
	ifstream tp("transparent.xml");
	if (!tp) { transparent = false; }
	else { transparent = true; }

	ifstream tpWM("transparentWhenMaximised.xml");
	if(!tpWM) { tpWhenMax = false; }
	else { tpWhenMax = true; }
	
	while (true) {

		secondTbVec.clear();
		while (otherTb = FindWindowEx(0, otherTb, "Shell_SecondaryTrayWnd", ""))
			secondTbVec.push_back(otherTb);

		mainTb = FindWindowA("Shell_TrayWnd", NULL);
		for( int a = 0; a < 10; a++) {
			for( int a = 0; a < 50; a++) {
				if(tpWhenMax)
					fgWin = GetForegroundWindow();
					::GetWindowPlacement(fgWin, &result);
					if(result.showCmd == 3) { transparent = false; }
					else { transparent = true; }
				SetWindowBlur(mainTb, transparent); 
				for(unsigned int i = 0; i < secondTbVec.size(); ++i) {
					SetWindowBlur(secondTbVec[i], transparent); //call the virtual function
				}
				Sleep((DWORD)10);}
		}
	}
	FreeLibrary(hModule);

}
