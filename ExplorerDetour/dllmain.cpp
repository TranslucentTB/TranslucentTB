#include "arch.h"
#include <windef.h>
#include <WinBase.h>
#include <WinUser.h>

#include "../detours/detours.h"
#include "constants.hpp"
#include "dlldata.hpp"
#include "hook.hpp"
#include "detourexception.h"
#include "detourtransaction.hpp"

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
		std::lock_guard guard(Hook::s_initLock);

		if (Hook::s_initState == Hook::InitializationState::InitializationDone)
		{
			try
			{
				DetourTransaction transaction;

				transaction.update_current_thread();
				transaction.detach(Hook::SetWindowCompositionAttribute, &Hook::SetWindowCompositionAttributeDetour);
				transaction.commit();

				Hook::s_initState = Hook::InitializationState::NotInitialized;
			}
			catch (const DetourException &err)
			{
				MessageBox(
					NULL,
					(L"Failed to remove detour: " + err.message()).c_str(),
					NAME L" Hook - Error",
					MB_ICONERROR | MB_OK | MB_SETFOREGROUND
				);
			}
		}
	}
	}

	return TRUE;
}