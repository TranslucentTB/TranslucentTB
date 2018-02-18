#pragma once
#ifndef __CPICKER_H
#define __CPICKER_H

#include <windows.h>
#include "CPickerDll.h"

// Just some useful macros...
#define _IS_IN(min, max, x)  (((x)>(min)) && ((x)<(max)))

#define CLAMP(x, m, M){\
	if ((x)<(m)) (x) = (m);\
	if ((x)>(M)) (x) = (M);\
}

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

	private:
		HBITMAP hBmpBuffer;
		void *lpBits;
		int w, h;
};

LRESULT CALLBACK ColourPickerDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
void DrawCheckedRect(HWND hWnd, int r, int g, int b, int a, int cx, int cy);
void DrawCircle(HDC hcomp, int red, int green, int blue, float x, float y);
void DrawArrows(HDC hcomp, int width, int height, float y);
void UpdateValues(HWND hDlg, SColour col);

#endif