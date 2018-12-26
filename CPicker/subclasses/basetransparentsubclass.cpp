#include "basetransparentsubclass.h"
#include <WinUser.h>
#include <Uxtheme.h>

LRESULT BaseTransparentSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR)
{
	switch (uMsg)
	{
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);

		RECT rect;
		GetClientRect(hWnd, &rect);

		BP_PAINTPARAMS params = {
			sizeof(params),
			BPPF_ERASE
		};

		HDC hdcPaint = NULL;

		HPAINTBUFFER hBufferedPaint = BeginBufferedPaint(hdc, &rect, BPBF_TOPDOWNDIB, &params, &hdcPaint);

		DefSubclassProc(hWnd, WM_PRINTCLIENT, reinterpret_cast<WPARAM>(hdcPaint), PRF_CLIENT | PRF_CHECKVISIBLE);

		BufferedPaintSetAlpha(hBufferedPaint, &rect, 255);

		EndBufferedPaint(hBufferedPaint, true);

		EndPaint(hWnd, &ps);

		return 1;
	}

	case WM_NCDESTROY:
		RemoveWindowSubclass(hWnd, BaseTransparentSubclass, uIdSubclass);
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}
