#include "ccolourpicker.hpp"
#include <WinUser.h>

#include "alphaslidercontext.hpp"
#include "colorpreviewcontext.hpp"
#include "main.hpp"
#include "mainpickercontext.hpp"
#include "pickerdata.hpp"
#include "resource.h"

std::unordered_map<uint32_t *, HWND> CColourPicker::PickerMap;
// https://blogs.msdn.microsoft.com/oldnewthing/20041025-00/?p=37483/
// __ImageBase is already declared by atlbase.h, which is imported in pickerdata.hpp
const HINSTANCE CColourPicker::Instance = reinterpret_cast<HINSTANCE>(&__ImageBase);

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
		MainPickerContext c1;
		AlphaSliderContext a;
		ColorPreviewContext c;
		PickerData data = {
			this,
			{{
				{ &c1, IDC_COLOR },
				{ &a,  IDC_ALPHASLIDE },
				{ &c,  IDC_COLORS }
			}}
		};
		DialogBoxParam(Instance, MAKEINTRESOURCE(IDD_COLORPICKER), hParent, ColourPickerDlgProc, reinterpret_cast<LPARAM>(&data));
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