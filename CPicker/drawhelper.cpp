#include "drawhelper.hpp"
#include <d2d1helper.h>

#include "CPicker.h"

void DrawColorSlider(ID2D1RenderTarget *target, const HWND &hDlg, const float &r, const float &g, const float &b, const unsigned short &h, const uint8_t &s, const uint8_t &v)
{
	const DWORD backgroundColor = GetSysColor(COLOR_BTNFACE);
	const D2D1::ColorF backgroundColorF(GetRValue(backgroundColor) / 255.0f, GetGValue(backgroundColor) / 255.0f, GetBValue(backgroundColor) / 255.0f);

	target->BeginDraw();
	target->Clear(backgroundColorF);

	D2D1_COLOR_F top_color, bottom_color;

	// Check who is selected.
	// RED
	if (IsDlgButtonChecked(hDlg, IDC_R) == BST_CHECKED)
	{
		top_color = D2D1::ColorF(1, g, b);
		bottom_color = D2D1::ColorF(0, g, b);
	}
	// GREEN
	else if (IsDlgButtonChecked(hDlg, IDC_G) == BST_CHECKED)
	{
		top_color = D2D1::ColorF(r, 1, b);
		bottom_color = D2D1::ColorF(r, 0, b);
	}
	// BLUE
	else if (IsDlgButtonChecked(hDlg, IDC_B) == BST_CHECKED)
	{
		top_color = D2D1::ColorF(r, g, 1);
		bottom_color = D2D1::ColorF(r, g, 0);
	}
	// HUE
	else if (IsDlgButtonChecked(hDlg, IDC_H) == BST_CHECKED)
	{
		// TODO: This doesn't works
		SColour tempcol;

		tempcol.h = 359;
		tempcol.s = 100;
		tempcol.v = 100;
		tempcol.UpdateRGB();

		top_color = D2D1::ColorF(tempcol.r / 255.0f, tempcol.g / 255.0f, tempcol.b / 255.0f);

		tempcol.h = 0;
		tempcol.UpdateRGB();
		bottom_color = D2D1::ColorF(tempcol.r / 255.0f, tempcol.g / 255.0f, tempcol.b / 255.0f);
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
	}
	// VALUE
	else if (IsDlgButtonChecked(hDlg, IDC_V) == BST_CHECKED)
	{
		SColour tempcol;

		tempcol.h = h;
		tempcol.s = v;
		tempcol.v = 100;
		tempcol.UpdateRGB();

		top_color = D2D1::ColorF(tempcol.r / 255.0f, tempcol.g / 255.0f, tempcol.b / 255.0f);

		tempcol.v = 0;
		tempcol.UpdateRGB();
		bottom_color = D2D1::ColorF(tempcol.r / 255.0f, tempcol.g / 255.0f, tempcol.b / 255.0f);
	}

	DrawGradient(target, top_color, bottom_color, 5);
	
	target->EndDraw();
}

void DrawAlphaSlider(ID2D1RenderTarget * target, const D2D1_COLOR_F &color)
{
	const DWORD backgroundColor = GetSysColor(COLOR_BTNFACE);
	const D2D1::ColorF backgroundColorF(GetRValue(backgroundColor) / 255.0f, GetGValue(backgroundColor) / 255.0f, GetBValue(backgroundColor) / 255.0f);

	target->BeginDraw();
	target->Clear(backgroundColorF);

	// TODO: Fill checkerboard

	DrawGradient(target, D2D1::ColorF(color.r, color.g, color.b, 1), D2D1::ColorF(color.r, color.g, color.b, 0), 5);

	target->EndDraw();
}

void DrawGradient(ID2D1RenderTarget *target, const D2D1_COLOR_F &top, const D2D1_COLOR_F &bottom, const uint8_t &border_size)
{
	const D2D1_SIZE_F size = target->GetSize();

	D2D1_GRADIENT_STOP gradientStops[2];
	gradientStops[0].color = top;
	gradientStops[0].position = 0.0f;
	gradientStops[1].color = bottom;
	gradientStops[1].position = 1.0f;

	CComPtr<ID2D1GradientStopCollection> pGradientStops;
	target->CreateGradientStopCollection(
		gradientStops,
		2,
		D2D1_GAMMA_1_0,
		D2D1_EXTEND_MODE_CLAMP,
		&pGradientStops
	);

	CComPtr<ID2D1LinearGradientBrush> brush;
	target->CreateLinearGradientBrush(
		D2D1::LinearGradientBrushProperties(
			D2D1::Point2F(0, 0),
			D2D1::Point2F(0, size.height)
		),
		pGradientStops,
		&brush
	);

	target->FillRectangle(D2D1::RectF(border_size, 0, size.width - border_size, size.height), brush);
}
