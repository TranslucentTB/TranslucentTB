#pragma once
#include <windef.h>

struct SColour
{
	unsigned short r, g, b;		// Red, green and blue
	unsigned short h, s, v;		// Hue, saturation and value
	unsigned short a;			// Alpha

	void UpdateRGB();			// Updates RGB from HSV
	void UpdateHSV();			// Updates HSV from RGB
};

class CColourPicker
{
	public:
		// Constructor
		_declspec(dllexport) CColourPicker(HWND hParentWindow);
		// Use true for IsRGB if passing RGBA or false if passing HSVA 
		_declspec(dllexport) CColourPicker(HWND hParentWindow, unsigned short r, unsigned short g, unsigned short b, unsigned short a, bool IsRGB);

		// Creates the colour picker dialog
		_declspec(dllexport) void CreateColourPicker();

		// Functions to set the colour components
		// NOTE: SetRGB automatically updates HSV and viceversa
		_declspec(dllexport) void SetRGB(unsigned short r, unsigned short g, unsigned short b);
		_declspec(dllexport) void SetHSV(unsigned short h, unsigned short s, unsigned short v);
		_declspec(dllexport) void SetAlpha(unsigned short a);
		
		// Some easy functions to retrieve the colour components
		_declspec(dllexport) SColour GetCurrentColour();
		_declspec(dllexport) SColour GetOldColour();

		void Revert();
		_declspec(dllexport) void UpdateOldColour();
		
		void SetParent(HWND _parent);
	private:
		// The current selected colour and the previous selected one
		SColour CurrCol, OldCol;
		HWND hParent;
};
