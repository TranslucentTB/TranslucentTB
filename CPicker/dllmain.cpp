#include "../TranslucentTB/arch.h"
#include <windef.h>
#include <WinBase.h>

#include "dlldata.hpp"

HINSTANCE DllData::m_hInst;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hinstDLL);
		DllData::m_hInst = hinstDLL;
		break;
	}

	return TRUE;
}