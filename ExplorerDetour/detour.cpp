#include "detour.hpp"
#include <libloaderapi.h>
#include <WinUser.h>

#include "constants.hpp"
#include "detourtransaction.hpp"
#include "window.hpp"

PFN_SET_WINDOW_COMPOSITION_ATTRIBUTE Detour::SetWindowCompositionAttribute =
	reinterpret_cast<PFN_SET_WINDOW_COMPOSITION_ATTRIBUTE>(GetProcAddress(GetModuleHandle(SWCA_DLL), SWCA_ORDINAL));
const std::wstring Detour::s_WorkerWindow = WORKER_WINDOW;
UINT Detour::s_RequestRefreshMessage = RegisterWindowMessage(WM_TTBHOOKREQUESTREFRESH);

BOOL Detour::SetWindowCompositionAttributeDetour(HWND hWnd, const WINDOWCOMPOSITIONATTRIBDATA *data) noexcept
{
	if (data->Attrib == WCA_ACCENT_POLICY && Window::Find(s_WorkerWindow, s_WorkerWindow).send_message(s_RequestRefreshMessage, 0, reinterpret_cast<LPARAM>(hWnd)))
	{
		return TRUE;
	}
	else
	{
		return SetWindowCompositionAttribute(hWnd, data);
	}
}

void Detour::Install()
{
	DetourTransaction transaction;

	transaction.update_all_threads();
	transaction.attach(SetWindowCompositionAttribute, SetWindowCompositionAttributeDetour);
	transaction.commit();
}

void Detour::Uninstall()
{
	DetourTransaction transaction;

	transaction.update_all_threads();
	transaction.detach(SetWindowCompositionAttribute, SetWindowCompositionAttributeDetour);
	transaction.commit();
}
