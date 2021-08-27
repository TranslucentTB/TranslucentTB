#include "swcadetour.hpp"
#include <libloaderapi.h>
#include <WinUser.h>
#include <wil/result.h>

#include "constants.hpp"
#include "detourtransaction.hpp"
#include "util/string_macros.hpp"

SWCADetour::unique_module_failfast SWCADetour::s_User32;
PFN_SET_WINDOW_COMPOSITION_ATTRIBUTE SWCADetour::SetWindowCompositionAttribute;
UINT SWCADetour::s_RequestAttribute;
bool SWCADetour::s_DetourInstalled;

void SWCADetour::FreeLibraryFailFast(HMODULE hModule) noexcept
{
	FAIL_FAST_IF_WIN32_BOOL_FALSE(FreeLibrary(hModule));
}

BOOL WINAPI SWCADetour::FunctionDetour(HWND hWnd, const WINDOWCOMPOSITIONATTRIBDATA *data) noexcept
{
	if (data && data->Attrib == WCA_ACCENT_POLICY)
	{
		if (const auto worker = FindWindow(TTB_WORKERWINDOW.c_str(), TTB_WORKERWINDOW.c_str()))
		{
			// avoid freezing Explorer if our main process is frozen
			DWORD_PTR result = 0;
			if (SendMessageTimeout(worker, s_RequestAttribute, 0, reinterpret_cast<LPARAM>(hWnd), SMTO_ABORTIFHUNG | SMTO_BLOCK | SMTO_ERRORONEXIT, 50, &result) && result)
			{
				return true;
			}
		}
	}

	return SetWindowCompositionAttribute(hWnd, data);
}

void SWCADetour::Install() noexcept
{
	if (!s_User32)
	{
		s_User32.reset(LoadLibraryEx(L"user32.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32));
		FAIL_FAST_LAST_ERROR_IF_NULL(s_User32);
	}

	if (!SetWindowCompositionAttribute)
	{
		SetWindowCompositionAttribute = reinterpret_cast<PFN_SET_WINDOW_COMPOSITION_ATTRIBUTE>(GetProcAddress(s_User32.get(), UTIL_STRINGIFY_UTF8(SetWindowCompositionAttribute)));
		FAIL_FAST_LAST_ERROR_IF(!SetWindowCompositionAttribute);
	}

	if (!s_RequestAttribute)
	{
		s_RequestAttribute = RegisterWindowMessage(WM_TTBHOOKREQUESTREFRESH.c_str());
		FAIL_FAST_LAST_ERROR_IF(!s_RequestAttribute);
	}

	if (!s_DetourInstalled)
	{
		DetourTransaction transaction;
		transaction.update_all_threads();
		transaction.attach(SetWindowCompositionAttribute, FunctionDetour);
		transaction.commit();

		s_DetourInstalled = true;
	}
}

void SWCADetour::Uninstall() noexcept
{
	if (s_DetourInstalled)
	{
		DetourTransaction transaction;
		transaction.update_all_threads();
		transaction.detach(SetWindowCompositionAttribute, FunctionDetour);
		transaction.commit();

		s_DetourInstalled = false;
	}
}
