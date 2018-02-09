#ifndef __CPICKERDLL_H
#define __CPICKERDLL_H

#include <dwmapi.h>

// Just some useful macros...
#define EXPORT _declspec (dllexport)
#define _IS_IN(min, max, x)  (((x)>(min)) && ((x)<(max)))

#define CLAMP(x, m, M){\
	if ((x)<(m)) (x) = (m);\
	if ((x)>(M)) (x) = (M);\
}

#define MAX(a, b, c) ((a)>(b)? ((a)>(c)?(a):(c)) : ((b)>(c)?(b):(c)))
#define MIN(a, b, c) ((a)<(b)? ((a)<(c)?(a):(c)) : ((b)<(c)?(b):(c)))
#define WIDTH(r) ((r).right-(r).left)
#define HEIGHT(r) ((r).bottom-(r).top)
 

// No alpha slider displayed
#define CP_NO_ALPHA			0
// Alpha slider displayed
#define CP_USE_ALPHA		1
// Alpha slider displayed but unusable
#define CP_DISABLE_ALPHA	2

struct SColour
	{
	unsigned short r, g, b;		// Red, green and blue
	unsigned short h, s, v;		// Hue, saturation and value
	unsigned short a;			// Alpha

	void UpdateRGB ();			// Updates RGB from HSV
	void UpdateHSV ();			// Updates HSV from RGB
	};

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
		int w,h;
	};

class CColourPicker
	{
	public:
		// Constructor
		EXPORT CColourPicker(HWND hParentWindow);
		// Use true for IsRGB if passing RGBA or false if passing HSVA 
		EXPORT CColourPicker(HWND hParentWindow, unsigned short r, unsigned short g, unsigned short b, 
			unsigned short a, bool IsRGB);

		// Creates the colour picker dialog
		EXPORT void CreateColourPicker(short AlphaUsage);

		// Functions to set the colour components
		// NOTE: SetRGB automatically updates HSV and viceversa
		EXPORT void SetRGB(unsigned short r, unsigned short g, unsigned short b);
		EXPORT void SetHSV(unsigned short h, unsigned short s, unsigned short v);
		EXPORT void SetAlpha(unsigned short a);
		
		// Some easy functions to retrieve the colour components
		EXPORT SColour GetCurrentColour();
		EXPORT SColour GetOldColour();

		// Returns CP_NO_ALPHA if not using alpha, CP_USE_ALPHA if using or 
		// CP_DISABLE_ALPHA if alpha is not used but the slider is displayed
		short GetAlphaUsage();
		void Revert();
		EXPORT void UpdateOldColour();
		
    void SetParent(HWND _parent){ hParent=_parent;}
	private:
		// The current selected colour and the previous selected one
		SColour CurrCol, OldCol;
		short UseAlpha;
		
		HWND hParent;
	};

EXPORT void DrawCheckedRect(HWND hWnd, int r, int g, int b, int a, int cx, int cy);

extern PixelBuffer pbuffer;

LRESULT CALLBACK ColourPickerDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
void UpdateValues(HWND hDlg, struct SColour col);

#endif
