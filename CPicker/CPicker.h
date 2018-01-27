#ifndef __CPICKER_H
#define __CPICKER_H

#include <windows.h>

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

class CColourPicker
	{
	public:
		// Constructor
		CColourPicker(HWND hParentWindow);
		// Use true for IsRGB if passing RGBA or false if passing HSVA 
		CColourPicker(HWND hParentWindow, unsigned short r, unsigned short g, unsigned short b, 
			unsigned short a, bool IsRGB);

		// Creates the colour picker dialog
		void CreatecolourPicker(short AlphaUsage);

		// Functions to set the colour components
		// NOTE: SetRGB automatically updates HSV and viceversa
		void SetRGB(unsigned short r, unsigned short g, unsigned short b);
		void SetHSV(unsigned short h, unsigned short s, unsigned short v);
		void SetAlpha(unsigned short a);
		
		// Some easy functions to retrieve the colour components
		SColour GetCurrentColour();
		SColour GetOldColour();

	private:
		// The current selected colour and the previous selected one
		SColour CurrCol, OldCol;
		short UseAlpha;
		
		HWND hParent;
	};

void DrawCheckedRect(HWND hWnd, int r, int g, int b, int a, int cx, int cy);

#endif