#include "CPicker.h"
#include <WinUser.h>

#include "resource.h"

std::unordered_map<uint32_t *, HWND> CColourPicker::PickerMap;

CColourPicker::CColourPicker(uint32_t &value, HWND hParentWindow) : Value(value)
{
	const uint8_t a = (Value & 0xFF000000) >> 24;
	const uint8_t r = (Value & 0x00FF0000) >> 16;
	const uint8_t g = (Value & 0x0000FF00) >> 8;
	const uint8_t b = (Value & 0x000000FF);

	CurrCol.r = r;
	CurrCol.g = g;
	CurrCol.b = b;
	CurrCol.a = a;
	CurrCol.UpdateHSV();

	OldCol = CurrCol;
	
	hParent = hParentWindow;

	Value = value;
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

// Updates the RGB color from the HSV
void SColour::UpdateRGB()
{
	int conv;
	double hue, sat, val;
	int base;

	hue = (float)h / 100.0f;
	sat = (float)s / 100.0f;
	val = (float)v / 100.0f;

	if ((float)s == 0) // Acromatic color (gray). Hue doesn't mind.
		{
		conv = (unsigned short) (255.0f * val);
		r = b = g = conv;
		return;
		}
	
	base = (unsigned short)(255.0f * (1.0 - sat) * val);

	switch ((unsigned short)((float)h/60.0f))
	{
		case 0:
			r = (unsigned short)(255.0f * val);
			g = (unsigned short)((255.0f * val - base) * (h/60.0f) + base);
			b = base;
		break;

		case 1:
			r = (unsigned short)((255.0f * val - base) * (1.0f - ((h%60)/ 60.0f)) + base);
			g = (unsigned short)(255.0f * val);
			b = base;
		break;

		case 2:
			r = base;
			g = (unsigned short)(255.0f * val);
			b = (unsigned short)((255.0f * val - base) * ((h%60)/60.0f) + base);
		break;
		
		case 3:
			r = base;
			g = (unsigned short)((255.0f * val - base) * (1.0f - ((h%60) / 60.0f)) + base);
			b = (unsigned short)(255.0f * val);
		break;
		
		case 4:
			r = (unsigned short)((255.0f * val - base) * ((h%60) / 60.0f) + base);
			g = base;
			b = (unsigned short)(255.0f * val);
		break;
		
		case 5:
			r = (unsigned short)(255.0f * val);
			g = base;
			b = (unsigned short)((255.0f * val - base) * (1.0f - ((h%60) / 60.0f)) + base);
		break;
	}
}

// Updates the HSV color from the RGB
void SColour::UpdateHSV()
{
	unsigned short max, min, delta;
	short temp;
    
	max = MAX(r, g, b);
	min = MIN(r, g, b);
	delta = max-min;

    if (max == 0)
		{
		s = h = v = 0;
		return;
		}
    
	v = (unsigned short) ((double)max/255.0*100.0);
	s = (unsigned short) (((double)delta/max)*100.0);

	if (r == max)
		temp = (short)(60 * ((g-b) / (double) delta));
	else if (g == max)
		temp = (short)(60 * (2.0 + (b-r) / (double) delta));
	else
		temp = (short)(60 * (4.0 + (r-g) / (double) delta));
	
	if (temp<0)
		h = temp + 360;
	else
		h = temp;
}