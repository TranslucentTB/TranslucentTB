#include "drawroutines.hpp"

#include "CPicker.h"
#include "drawhelper.hpp"

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

		DrawArrows(target, 1.0 - r, 5);
	}
	// GREEN
	else if (IsDlgButtonChecked(hDlg, IDC_G) == BST_CHECKED)
	{
		top_color = D2D1::ColorF(r, 1, b);
		bottom_color = D2D1::ColorF(r, 0, b);

		DrawArrows(target, 1.0 - g, 5);
	}
	// BLUE
	else if (IsDlgButtonChecked(hDlg, IDC_B) == BST_CHECKED)
	{
		top_color = D2D1::ColorF(r, g, 1);
		bottom_color = D2D1::ColorF(r, g, 0);

		DrawArrows(target, 1.0 - b, 5);
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

		DrawArrows(target, 1.0 - (h / 359), 5);
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

		DrawArrows(target, 1.0 - (s / 100), 5);
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

		DrawArrows(target, 1.0 - (v / 100), 5);
	}

	DrawGradient(target, top_color, bottom_color, 5);

	target->EndDraw();
}

void DrawAlphaSlider(ID2D1RenderTarget *target, const D2D1_COLOR_F &color, const float &a)
{
	const DWORD backgroundColor = GetSysColor(COLOR_BTNFACE);
	const D2D1_SIZE_F size = target->GetSize();

	target->BeginDraw();
	target->Clear(D2D1::ColorF(GetRValue(backgroundColor) / 255.0f, GetGValue(backgroundColor) / 255.0f, GetBValue(backgroundColor) / 255.0f));

	CComPtr<ID2D1SolidColorBrush> brush;
	target->CreateSolidColorBrush(D2D1::ColorF(0, 0.3), &brush);

	DrawCheckerboard(target, brush, size, (size.width / 2) - 5, 5);

	DrawGradient(target, D2D1::ColorF(color.r, color.g, color.b, 1), D2D1::ColorF(color.r, color.g, color.b, 0), 5);

	DrawArrows(target, a, 5);

	target->EndDraw();
}

void DrawColorIndicator(ID2D1RenderTarget *target, const SColour &color)
{
	target->BeginDraw();
	target->Clear(D2D1::ColorF(D2D1::ColorF::White));

	const D2D1_SIZE_F size = target->GetSize();

	CComPtr<ID2D1SolidColorBrush> brush;
	target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &brush);
	DrawCheckerboard(target, brush, size, 10);
	brush.Release();

	target->CreateSolidColorBrush(D2D1::ColorF(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f), &brush);
	target->FillRectangle(D2D1::RectF(0, 0, size.width, size.height), brush);

	target->EndDraw();
}