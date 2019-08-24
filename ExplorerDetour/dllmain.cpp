#include "arch.h"
#include <debugapi.h>
#include <libloaderapi.h>
#include <windef.h>
#include <process.h>

#include "detour.hpp"
#include "detourexception.hpp"

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID) noexcept
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
	{
		DisableThreadLibraryCalls(hinstDLL);
		try
		{
			Detour::Install();
		}
		catch (const DetourException &err)
		{
			OutputDebugString(err.message().c_str());
			return FALSE;
		}
		break;
	}

	case DLL_PROCESS_DETACH:
	{
		Detour::Uninstall();
		break;
	}
	}

	return TRUE;
}
