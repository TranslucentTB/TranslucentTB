#include "CColourPicker.hpp"
#include <WinUser.h>

#include "main.hpp"
#include "PickerData.hpp"
#include "resource.h"

std::unordered_map<uint32_t *, HWND> CColourPicker::PickerMap;

CColourPicker::CColourPicker(uint32_t &value, HWND hParentWindow) : Value(value), hParent(hParentWindow)
{
	CurrCol.r = (Value & 0x00FF0000) >> 16;
	CurrCol.g = (Value & 0x0000FF00) >> 8;
	CurrCol.b = (Value & 0x000000FF);
	CurrCol.a = (Value & 0xFF000000) >> 24;
	CurrCol.UpdateHSV();

	OldCol = CurrCol;
}

void CColourPicker::CreateColourPicker()
{
	if (PickerMap.count(&Value) == 0)
	{
		PickerData data;
		data.picker = this;
		DialogBoxParam(GetModuleHandle(L"CPicker.dll"), MAKEINTRESOURCE(IDD_COLORPICKER), hParent, ColourPickerDlgProc, reinterpret_cast<LPARAM>(&data));
		PickerMap.erase(&Value);
	}
	else
	{
		SetForegroundWindow(PickerMap.at(&Value));
	}
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