#include "ccolourpicker.hpp"

#include "gui.hpp"

CColourPicker::CColourPicker(uint32_t &value, HWND hParentWindow) : Value(value), hParent(hParentWindow)
{
	CurrCol.r = (Value & 0x00FF0000) >> 16;
	CurrCol.g = (Value & 0x0000FF00) >> 8;
	CurrCol.b = (Value & 0x000000FF);
	CurrCol.a = (Value & 0xFF000000) >> 24;
	CurrCol.UpdateHSV();

	OldCol = CurrCol;
}

HRESULT CColourPicker::CreateColourPicker()
{
	return GUI::CreateGUI(this, Value, hParent);
}

void CColourPicker::SetRGB(uint8_t r, uint8_t g, uint8_t b)
{
	CurrCol.r = r;
	CurrCol.g = g;
	CurrCol.b = b;

	CurrCol.UpdateHSV();

	UpdateValue();
}

void CColourPicker::SetHSV(unsigned short h, uint8_t s, uint8_t v)
{
	// Clamp hue values to 359, sat and val to 100
	CurrCol.h = min(h, 359);
	CurrCol.s = min(s, 100);
	CurrCol.v = min(v, 100);

	CurrCol.UpdateRGB();

	UpdateValue();
}

void CColourPicker::SetAlpha(uint8_t a)
{
	CurrCol.a = a;

	UpdateValue();
}

const SColour &CColourPicker::GetCurrentColour()
{
	return CurrCol;
}

const SColour &CColourPicker::GetOldColour()
{
	return OldCol;
}

void CColourPicker::UpdateOldColour()
{
	OldCol = CurrCol;
}

void CColourPicker::UpdateValue()
{
	Value = (CurrCol.a << 24) + (CurrCol.r << 16) + (CurrCol.g << 8) + CurrCol.b;
}