#pragma once
#ifndef __CPICKERDLL_H
#define __CPICKERDLL_H

#include <windef.h>

#define EXPORT _declspec(dllexport)

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
		EXPORT CColourPicker(HWND hParentWindow);
		// Use true for IsRGB if passing RGBA or false if passing HSVA 
		EXPORT CColourPicker(HWND hParentWindow, unsigned short r, unsigned short g, unsigned short b, unsigned short a, bool IsRGB);

		// Creates the colour picker dialog
		EXPORT void CreateColourPicker();

		// Functions to set the colour components
		// NOTE: SetRGB automatically updates HSV and viceversa
		EXPORT void SetRGB(unsigned short r, unsigned short g, unsigned short b);
		EXPORT void SetHSV(unsigned short h, unsigned short s, unsigned short v);
		EXPORT void SetAlpha(unsigned short a);
		
		// Some easy functions to retrieve the colour components
		EXPORT SColour GetCurrentColour();
		EXPORT SColour GetOldColour();

		void Revert();
		EXPORT void UpdateOldColour();
		
		void SetParent(HWND _parent){ hParent=_parent;}
	private:
		// The current selected colour and the previous selected one
		SColour CurrCol, OldCol;
		HWND hParent;
};

#endif
