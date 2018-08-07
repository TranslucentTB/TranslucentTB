#pragma once
#include "../TranslucentTB/arch.h"
#include <algorithm>
#include <cstdint>
#include <windef.h>

#include "scolour.hpp"

class __declspec(
#ifdef _CPICKER_DLL
dllexport
#else
dllimport
#endif
) CColourPicker {

public:
#pragma warning(push)
#pragma warning(disable: 4244)
	constexpr CColourPicker(uint32_t &value, HWND hParentWindow = NULL) : Value(value), CurrCol(), OldCol(), hParent(hParentWindow)
	{
		CurrCol.r = (Value & 0x00FF0000) >> 16;
		CurrCol.g = (Value & 0x0000FF00) >> 8;
		CurrCol.b = (Value & 0x000000FF);
		CurrCol.a = (Value & 0xFF000000) >> 24;
		CurrCol.UpdateHSV();

		OldCol = CurrCol;
	}
#pragma warning(pop)

	// Creates the colour picker dialog
	HRESULT CreateColourPicker();

	// Functions to set the colour components
	// NOTE: SetRGB automatically updates HSV and viceversa
	constexpr void SetRGB(uint8_t r, uint8_t g, uint8_t b)
	{
		CurrCol.r = r;
		CurrCol.g = g;
		CurrCol.b = b;

		CurrCol.UpdateHSV();

		UpdateValue();
	}

	constexpr void SetHSV(uint16_t h, uint8_t s, uint8_t v)
	{
		// Clamp hue values to 359, sat and val to 100
		CurrCol.h = std::clamp<uint16_t>(h, 0, 359);
		CurrCol.s = std::clamp<uint8_t>(s, 0, 100);
		CurrCol.v = std::clamp<uint8_t>(v, 0, 100);

		CurrCol.UpdateRGB();

		UpdateValue();
	}

	constexpr void SetAlpha(uint8_t a)
	{
		CurrCol.a = a;

		UpdateValue();
	}

	// Some easy functions to retrieve the colour components
	constexpr const SColour &GetCurrentColour() const { return CurrCol; }
	constexpr const SColour &GetOldColour() const { return OldCol; }

	constexpr void UpdateOldColour()
	{
		OldCol = CurrCol;
	}

private:
	constexpr void UpdateValue()
	{
		Value = (CurrCol.a << 24) + (CurrCol.r << 16) + (CurrCol.g << 8) + CurrCol.b;
	}

	uint32_t &Value;
	// The current selected colour and the previous selected one
	SColour CurrCol, OldCol;
	HWND hParent;
};