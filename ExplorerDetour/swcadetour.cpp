#include "swcadetour.hpp"
#include <heapapi.h>
#include <libloaderapi.h>
#include <WinUser.h>

#include "constants.hpp"
#include "detourtransaction.hpp"
#include "util/abort.hpp"
#include "util/string_macros.hpp"

HMODULE SWCADetour::s_User32;
PFN_SET_WINDOW_COMPOSITION_ATTRIBUTE SWCADetour::SetWindowCompositionAttribute;
UINT SWCADetour::s_RequestAttribute;
HANDLE SWCADetour::s_Heap;
bool SWCADetour::s_DetourInstalled;

BOOL WINAPI SWCADetour::FunctionDetour(HWND hWnd, const WINDOWCOMPOSITIONATTRIBDATA *data) noexcept
{
	if (data->Attrib == WCA_ACCENT_POLICY)
	{
		if (const auto worker = FindWindow(WORKER_WINDOW.c_str(), WORKER_WINDOW.c_str()))
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
		s_User32 = LoadLibraryEx(L"user32.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
		if (!s_User32) [[unlikely]]
		{
			Util::QuickAbort();
		}
	}

	if (!SetWindowCompositionAttribute)
	{
		SetWindowCompositionAttribute = reinterpret_cast<PFN_SET_WINDOW_COMPOSITION_ATTRIBUTE>(GetProcAddress(s_User32, UTIL_STRINGIFY_UTF8(SetWindowCompositionAttribute)));
		if (!SetWindowCompositionAttribute) [[unlikely]]
		{
			Util::QuickAbort();
		}
	}

	if (!s_RequestAttribute)
	{
		s_RequestAttribute = RegisterWindowMessage(WM_TTBHOOKREQUESTREFRESH.c_str());
		if (!s_RequestAttribute) [[unlikely]]
		{
			Util::QuickAbort();
		}
	}

	if (!s_Heap)
	{
		s_Heap = HeapCreate(HEAP_NO_SERIALIZE, 0, 0);
		if (!s_Heap) [[unlikely]]
		{
			Util::QuickAbort();
		}
	}

	if (!s_DetourInstalled)
	{
		DetourTransaction transaction;
		if (transaction.begin() &&
			transaction.update_all_threads() &&
			transaction.attach(SetWindowCompositionAttribute, FunctionDetour) &&
			transaction.commit())
		{
			s_DetourInstalled = true;
		}
		else
		{
			Util::QuickAbort();
		}
	}
}

void SWCADetour::Uninstall() noexcept
{
	if (s_DetourInstalled)
	{
		DetourTransaction transaction;
		if (transaction.begin() &&
			transaction.update_all_threads() &&
			transaction.detach(SetWindowCompositionAttribute, FunctionDetour) &&
			transaction.commit())
		{
			s_DetourInstalled = false;
		}
		else
		{
			Util::QuickAbort();
		}
	}

	if (s_Heap)
	{
		if (HeapDestroy(s_Heap))
		{
			s_Heap = nullptr;
		}
		else
		{
			Util::QuickAbort();
		}
	}

	if (s_User32)
	{
		if (FreeLibrary(s_User32))
		{
			s_User32 = nullptr;
		}
		else
		{
			Util::QuickAbort();
		}
	}
}
