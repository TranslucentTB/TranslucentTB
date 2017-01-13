#include <windows.h>
#include <fstream>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <string>

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

BOOL transparent, tpWhenMax, tpWhenFgMax;
std::vector<BOOL> transparentVec;
char str[256];

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


BOOL CALLBACK EnumWindowsProcess(HWND hWnd, LPARAM lParam) {

	WINDOWPLACEMENT result = {};
	result.length = sizeof(WINDOWPLACEMENT);

	::GetWindowPlacement(hWnd, &result);
	if(result.showCmd == 3) { 
		transparent = false; 
	}

	return true;

}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreInst, LPSTR pCmdLine, int nCmdShow)
{
	HWND mainTb, otherTb, fgWin;
	unsigned int monitors = 1;
	WINDOWPLACEMENT result = {};
	HMONITOR _monitor;
	MONITORINFOEX hMonInfo = {sizeof(MONITORINFOEX)};
	result.length = sizeof(WINDOWPLACEMENT);
	std::vector<HWND> secondTbVec;
	tpWhenMax = true;
	
	fgWin = GetForegroundWindow();
	::GetWindowPlacement(fgWin, &result);
	
	sprintf_s(str, "StartMonitors: %d !", monitors);
	OutputDebugString(str);
	
	while (true) {
		monitors = 1;
		secondTbVec.clear();	
		transparentVec.clear();	
		mainTb = FindWindowA("Shell_TrayWnd", NULL);
		while (otherTb = FindWindowEx(0, otherTb, "Shell_SecondaryTrayWnd", ""))
			monitors++;
			secondTbVec.push_back(otherTb);
			
		if GetMonitorInfo(monitors[0], &hMonInfo) {		
			sprintf_s(str, "EndMonitors: %d ", hMonInfo.szDevice);
			OutputDebugString(str);
		}

		transparentVec.resize(monitors);
		transparentVec[0] = transparent;
		for(unsigned int i = 0; i < secondTbVec.size(); ++i) {
			_monitor = MonitorFromWindow(secondTbVec[i], MONITOR_DEFAULTTONEAREST);
		}

		for( int a = 0; a < 5; a++) {
			for( int b = 0; b < 100; b++) {
				if(tpWhenMax) {
					transparent = true;
					EnumWindows(EnumWindowsProcess, NULL); 
				}
				if(tpWhenFgMax) {
					fgWin = GetForegroundWindow();
					::GetWindowPlacement(fgWin, &result);
					if(result.showCmd == 3) { transparent = false; }
					else {transparent = true;}
				}

				SetWindowBlur(mainTb, transparent); 
				for(unsigned int i = 0; i < secondTbVec.size(); ++i) {
					SetWindowBlur(secondTbVec[i], transparent);
				}
				Sleep((DWORD)10);}
		}
	}
	FreeLibrary(hModule);

}
