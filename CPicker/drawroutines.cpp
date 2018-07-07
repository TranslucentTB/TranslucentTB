#include "drawroutines.hpp"
#include <atlbase.h>

#include "drawhelper.hpp"
#include "resource.h"

void DrawColorSlider(ID2D1RenderTarget *target, ID2D1SolidColorBrush *brush, ID2D1LinearGradientBrush *hue, const HWND &hDlg, const float &r, const float &g, const float &b, const unsigned short &h, const uint8_t &s, const uint8_t &v)
{
	const DWORD backgroundColor = GetSysColor(COLOR_BTNFACE);
	const D2D1_SIZE_F size = target->GetSize();
	const float border_size = size.width / 4.0f;

	target->BeginDraw();
	target->Clear(D2D1::ColorF(GetRValue(backgroundColor) / 255.0f, GetGValue(backgroundColor) / 255.0f, GetBValue(backgroundColor) / 255.0f));

	D2D1_COLOR_F top_color, bottom_color;
	D2D1_COLOR_F arrow_color = D2D1::ColorF(r, g, b);
	float arrow_position;

	// Check who is selected.
	// RED
	if (IsDlgButtonChecked(hDlg, IDC_R) == BST_CHECKED)
	{
		top_color = D2D1::ColorF(1.0f, g, b);
		bottom_color = D2D1::ColorF(0.0f, g, b);

		arrow_position = 1.0f - r;
	}
	// GREEN
	else if (IsDlgButtonChecked(hDlg, IDC_G) == BST_CHECKED)
	{
		top_color = D2D1::ColorF(r, 1.0f, b);
		bottom_color = D2D1::ColorF(r, 0.0f, b);

		arrow_position = 1.0f - g;
	}
	// BLUE
	else if (IsDlgButtonChecked(hDlg, IDC_B) == BST_CHECKED)
	{
		top_color = D2D1::ColorF(r, g, 1.0f);
		bottom_color = D2D1::ColorF(r, g, 0.0f);

		arrow_position = 1.0f - b;
	}
	// HUE
	else if (IsDlgButtonChecked(hDlg, IDC_H) == BST_CHECKED)
	{
		target->FillRectangle(D2D1::RectF(border_size, 0.0f, size.width - border_size, size.height), hue);

		arrow_position = 1.0f - (h / 359.0f);
		SColour temp;
		temp.h = h;
		temp.s = 100;
		temp.v = 100;
		temp.UpdateRGB();
		arrow_color = D2D1::ColorF(temp.r / 255.0f, temp.g / 255.0f, temp.b / 255.0f);
	}
	// SATURATION
	else if (IsDlgButtonChecked(hDlg, IDC_S) == BST_CHECKED)
	{
		SColour tempcol;

		tempcol.h = h;
		tempcol.s = 100;
		tempcol.v = v;
		tempcol.UpdateRGB();

		top_color = D2D1::ColorF(tempcol.r / 255.0f, tempcol.g / 255.0f, tempcol.b / 255.0f);

		tempcol.s = 0;
		tempcol.UpdateRGB();
		bottom_color = D2D1::ColorF(tempcol.r / 255.0f, tempcol.g / 255.0f, tempcol.b / 255.0f);

		arrow_position = 1.0f - (s / 100.0f);
	}
	// VALUE
	else if (IsDlgButtonChecked(hDlg, IDC_V) == BST_CHECKED)
	{
		SColour tempcol;

		tempcol.h = h;
		tempcol.s = s;
		tempcol.v = 100;
		tempcol.UpdateRGB();

		top_color = D2D1::ColorF(tempcol.r / 255.0f, tempcol.g / 255.0f, tempcol.b / 255.0f);

		tempcol.v = 0;
		tempcol.UpdateRGB();
		bottom_color = D2D1::ColorF(tempcol.r / 255.0f, tempcol.g / 255.0f, tempcol.b / 255.0f);

		arrow_position = 1.0f - (v / 100.0f);
	}

	// Hue has its own drawing code (a gradient with 20 steps), because it loops over the whole color space.
	// Meaning that if we use top_color and bottom_color it'll just appear as a big single-colored slider.
	if (IsDlgButtonChecked(hDlg, IDC_H) != BST_CHECKED)
	{
		DrawGradient(target, size, top_color, bottom_color, border_size);
	}

	brush->SetColor(arrow_color);
	DrawArrows(target, size, arrow_position, border_size, brush);

	target->EndDraw();
}