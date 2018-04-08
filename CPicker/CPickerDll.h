#pragma once
#include <cstdint>
#include <windef.h>

struct SColour
{
	// Red, green and blue
	uint8_t r, g, b;

	// Hue, saturation and value
	unsigned short h;
	uint8_t s, v;

	// Alpha
	uint8_t a;

	void UpdateRGB();			// Updates RGB from HSV
	void UpdateHSV();			// Updates HSV from RGB
};

class CColourPicker
{
	public:
		// Use true for IsRGB if passing RGBA or false if passing HSVA 
		_declspec(dllexport) CColourPicker(unsigned short r = 0, uint8_t g = 0, uint8_t b = 0, uint8_t a = 255, bool IsRGB = true, HWND hParentWindow = NULL);

		// Creates the colour picker dialog
		_declspec(dllexport) void CreateColourPicker();

		// Functions to set the colour components
		// NOTE: SetRGB automatically updates HSV and viceversa
		_declspec(dllexport) void SetRGB(uint8_t r, uint8_t g, uint8_t b);
		_declspec(dllexport) void SetHSV(unsigned short h, uint8_t s, uint8_t v);
		_declspec(dllexport) void SetAlpha(uint8_t a);
		
		// Some easy functions to retrieve the colour components
		_declspec(dllexport) SColour GetCurrentColour();
		_declspec(dllexport) SColour GetOldColour();

		_declspec(dllexport) void UpdateOldColour();
	private:
		// The current selected colour and the previous selected one
		SColour CurrCol, OldCol;
		HWND hParent;
};
