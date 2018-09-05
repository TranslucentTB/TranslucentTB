#include "../TranslucentTB/arch.h"
#include <windef.h>
#include <WinBase.h>

#include "../contrib/detours.h"
#include "dlldata.hpp"
#include "hook.hpp"

HINSTANCE DllData::m_hInst;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hinstDLL);
		DllData::m_hInst = hinstDLL;
		break;

	case DLL_PROCESS_DETACH:
	{
		std::lock_guard guard(Hook::m_initDoneLock);

		if (Hook::m_initDone)
		{
			// Can't handle errors, we're just about to detach. Worse case explorer will crash and restart automatically.
			DetourTransactionBegin();
			DetourUpdateThread(GetCurrentThread());
			DetourDetach(reinterpret_cast<void **>(&Hook::SetWindowCompositionAttribute), reinterpret_cast<void *>(Hook::SetWindowCompositionAttributeDetour));
			DetourTransactionCommit();

			Hook::m_initDone = false;
		}
	}
	}

	return TRUE;
}