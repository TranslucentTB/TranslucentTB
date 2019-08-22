#include "arch.h"
#include <windef.h>
#include <WinBase.h>
#include <WinUser.h>

#include "detourexception.hpp"
#include "detourtransaction.hpp"
#include "hook.hpp"
#include "window.hpp"

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID) noexcept
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hinstDLL);
		break;

	case DLL_PROCESS_DETACH:
		if (Hook::s_IsHooked.test_and_set())
		{
			DetourTransaction transaction;

			transaction.update_current_thread();
			transaction.detach(Hook::SetWindowCompositionAttribute, Hook::SetWindowCompositionAttributeDetour);
			transaction.commit();
		}
		Hook::s_IsHooked.clear();
		break;
	}

	return TRUE;
}
