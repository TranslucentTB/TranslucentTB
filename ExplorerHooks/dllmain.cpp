#include "arch.h"
#include <libloaderapi.h>
#include <windef.h>

#include "explorerhooks.hpp"
#include "swcadetour.hpp"
#include "timelinevisibilitymonitor.hpp"

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
			TimelineVisibilityMonitor::Install(hinstDLL);
		}
		break;

	case DLL_PROCESS_DETACH:
		if (payload)
		{
			TimelineVisibilityMonitor::Uninstall(hinstDLL);
			SWCADetour::Uninstall();

			ExplorerHooks::FreeExplorerPayload(payload);
			payload = nullptr;
		}
		break;
	}

	return true;
}
