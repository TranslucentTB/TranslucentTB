#include "../TranslucentTB/arch.h"
#include <windef.h>
#include <WinBase.h>
#include <WinUser.h>

#include "dlldata.hpp"
#include "resource.h"

HINSTANCE DllData::m_hInst;
RECT DllData::m_size;

RECT DllData::CalculateDialogSize(HINSTANCE hInstance)
{
	HRSRC dialog = FindResource(hInstance, MAKEINTRESOURCE(IDD_COLORPICKER), RT_DIALOG);
	HGLOBAL res = LoadResource(hInstance, dialog);
	auto dialogTemplate = reinterpret_cast<const DLGTEMPLATEEX *>(LockResource(res));

	const RECT size = {
		dialogTemplate->x,
		dialogTemplate->y,
		dialogTemplate->x + dialogTemplate->cx,
		dialogTemplate->y + dialogTemplate->cy
	};

	// Unlocking resources is not necessary anymore, UnlockResource omitted.
	FreeResource(res);

	return size;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hinstDLL);
		DllData::m_hInst = hinstDLL;
		DllData::m_size = DllData::CalculateDialogSize(hinstDLL);
		break;
	}

	return TRUE;
}