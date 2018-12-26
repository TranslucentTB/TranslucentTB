#include "nooutlinebuttonsubclass.h"
#include <WinUser.h>
#include <CommCtrl.h>

LRESULT NoOutlineButtonSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR)
{
	switch (uMsg)
	{
	case WM_SETFOCUS:
		return 0;

	case WM_NCDESTROY:
		RemoveWindowSubclass(hWnd, NoOutlineButtonSubclass, uIdSubclass);
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}
