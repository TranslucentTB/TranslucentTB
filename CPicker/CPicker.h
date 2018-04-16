#pragma once
#include <atlbase.h>
#include <d2d1.h>
#include <unordered_map>

#include "../TranslucentTB/arch.h"
#include "CPickerDll.h"
#include "resource.h"

// Just some useful macros...
#define _IS_IN(min, max, x)  (((x)>(min)) && ((x)<(max)))
#define MAX(a, b, c) ((a)>(b)? ((a)>(c)?(a):(c)) : ((b)>(c)?(b):(c)))
#define MIN(a, b, c) ((a)<(b)? ((a)<(c)?(a):(c)) : ((b)<(c)?(b):(c)))
#define WIDTH(r) ((r).right-(r).left)
#define HEIGHT(r) ((r).bottom-(r).top)

struct PixelBuffer
{
	public:
		PixelBuffer();
		void Create(int _w, int _h);
		void Destroy();
		void SetPixel(int x, int y, unsigned int color);
		void Display(HDC dc);
		~PixelBuffer();

	private:
		HDC hdc;
		HBITMAP hBmpBuffer;
		void *lpBits;
		int w, h;
};

struct PickerData
{
	CColourPicker *picker;
	bool changing_text;
	CComPtr<ID2D1Factory> factory;
	CComPtr<ID2D1HwndRenderTarget> targetC1;
	CComPtr<ID2D1HwndRenderTarget> targetC2;
	CComPtr<ID2D1HwndRenderTarget> targetA;
	CComPtr<ID2D1HwndRenderTarget> targetCC;
	CComPtr<ID2D1HwndRenderTarget> targetOC;
};

int CALLBACK ColourPickerDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
void DrawCircle(HDC hcomp, int red, int green, int blue, float x, float y);
void UpdateValues(HWND hDlg, const SColour &col, bool &changing_text);

const std::unordered_map<unsigned int, std::pair<unsigned int, unsigned int>> SLIDER_MAP = {
	{ IDC_RED,			{ IDC_RSLIDER, 255 } },
	{ IDC_GREEN,		{ IDC_GSLIDER, 255 } },
	{ IDC_BLUE,			{ IDC_BSLIDER, 255 } },
	{ IDC_ALPHA,		{ IDC_ASLIDER, 255 } },
	{ IDC_HUE,			{ IDC_HSLIDER, 359 } },
	{ IDC_SATURATION,	{ IDC_SSLIDER, 100 } },
	{ IDC_VALUE,		{ IDC_VSLIDER, 100 } },
	{ IDC_HEXCOL,		{ IDC_HEXSLIDER, 0xFFFFFFFF } }
};