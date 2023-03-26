#include "arch.h"
#include <libloaderapi.h>
#include <windef.h>
#include <processthreadsapi.h>
#include <detours/detours.h>

#include "constants.hpp"
#include "swcadetour.hpp"
#include "taskviewvisibilitymonitor.hpp"

void *payload = nullptr;

BOOL WINAPI DllMain(HINSTANCE, DWORD fdwReason, LPVOID) noexcept
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		// Are we in Explorer?
		payload = DetourFindPayloadEx(EXPLORER_PAYLOAD, nullptr);
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
		}
		break;
	}

	return true;
}
