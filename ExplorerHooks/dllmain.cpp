#include "arch.h"
#include <libloaderapi.h>
#include <windef.h>

#include "explorerhooks.hpp"
#include "swcadetour.hpp"
#include "taskviewvisibilitymonitor.hpp"

void *payload = nullptr;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID) noexcept
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		// ignore errors, worse case scenario we get called for new threads
		DisableThreadLibraryCalls(hinstDLL);

		// Are we in Explorer?
		payload = ExplorerHooks::FindExplorerPayload();
		if (payload)
		{
			// Install the things
			SWCADetour::Install();
			TaskViewVisibilityMonitor::Install();
		}
		break;

	case DLL_PROCESS_DETACH:
		if (payload)
		{
			TaskViewVisibilityMonitor::Uninstall();
			SWCADetour::Uninstall();

			ExplorerHooks::FreeExplorerPayload(payload);
			payload = nullptr;
		}
		break;
	}

	return true;
}
