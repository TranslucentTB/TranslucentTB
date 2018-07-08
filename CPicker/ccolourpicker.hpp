#pragma once
#include "../TranslucentTB/arch.h"
#include <cstdint>
#include <windef.h>

#include "scolour.hpp"

class __declspec(
#if defined(_CPICKER_DLL)
dllexport
#else
dllimport
#endif
) CColourPicker {

public:
	CColourPicker(uint32_t &value, HWND hParentWindow = NULL);

	// Creates the colour picker dialog
	HRESULT CreateColourPicker();

	// Functions to set the colour components
	// NOTE: SetRGB automatically updates HSV and viceversa
	void SetRGB(uint8_t r, uint8_t g, uint8_t b);
	void SetHSV(unsigned short h, uint8_t s, uint8_t v);
	void SetAlpha(uint8_t a);

	// Some easy functions to retrieve the colour components
	const SColour &GetCurrentColour();
	const SColour &GetOldColour();

	void UpdateOldColour();

private:
	void UpdateValue();

	uint32_t &Value;
	// The current selected colour and the previous selected one
	SColour CurrCol, OldCol;
	HWND hParent;
};