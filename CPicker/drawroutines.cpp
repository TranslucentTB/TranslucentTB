#include "drawroutines.hpp"
#include <atlbase.h>

#include "drawhelper.hpp"
#include "resource.h"

void DrawColorSlider(ID2D1RenderTarget *target, const HWND &hDlg, const float &r, const float &g, const float &b, const unsigned short &h, const uint8_t &s, const uint8_t &v)
{
	const DWORD backgroundColor = GetSysColor(COLOR_BTNFACE);

	target->BeginDraw();
	target->Clear(D2D1::ColorF(GetRValue(backgroundColor) / 255.0f, GetGValue(backgroundColor) / 255.0f, GetBValue(backgroundColor) / 255.0f));

	D2D1_COLOR_F top_color, bottom_color;

	// Check who is selected.
	// RED
	if (IsDlgButtonChecked(hDlg, IDC_R) == BST_CHECKED)
	{
		top_color = D2D1::ColorF(1, g, b);
		bottom_color = D2D1::ColorF(0, g, b);

		DrawArrows(target, 1.0 - r, 7);
	}
	// GREEN
	else if (IsDlgButtonChecked(hDlg, IDC_G) == BST_CHECKED)
	{
		top_color = D2D1::ColorF(r, 1, b);
		bottom_color = D2D1::ColorF(r, 0, b);

		DrawArrows(target, 1.0 - g, 7);
	}
	// BLUE
	else if (IsDlgButtonChecked(hDlg, IDC_B) == BST_CHECKED)
	{
		top_color = D2D1::ColorF(r, g, 1);
		bottom_color = D2D1::ColorF(r, g, 0);

		DrawArrows(target, 1.0 - b, 7);
	}
	// HUE
	else if (IsDlgButtonChecked(hDlg, IDC_H) == BST_CHECKED)
	{
		const D2D1_SIZE_F size = target->GetSize();

		D2D1_GRADIENT_STOP gradientStops[20];
		SColour tempcol;
		tempcol.h = 359;
		tempcol.s = 100;
		tempcol.v = 100;
		tempcol.UpdateRGB();

		const float step = 359 / 20.0f;

		for (int i = 0; i < 20; i++)
		{
			gradientStops[i].color = D2D1::ColorF(tempcol.r / 255.0f, tempcol.g / 255.0f, tempcol.b / 255.0f);
			gradientStops[i].position = i / 19.0f;

			tempcol.h -= step;
			tempcol.UpdateRGB();
		}

		CComPtr<ID2D1GradientStopCollection> pGradientStops;
		target->CreateGradientStopCollection(
			gradientStops,
			20,
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

		target->FillRectangle(D2D1::RectF(7, 0, size.width - 7, size.height), brush);

		DrawArrows(target, 1.0 - (h / 359.0f), 7);
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

		DrawArrows(target, 1.0 - (s / 100), 7);
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

		DrawArrows(target, 1.0 - (v / 100), 7);
	}

	// Hue has its own drawing code (a gradient with 20 steps), because it loops over the whole color space.
	// Meaning that if we use top_color and bottom_color it'll just appear as a big single-colored slider.
	if (IsDlgButtonChecked(hDlg, IDC_H) != BST_CHECKED)
	{
		DrawGradient(target, top_color, bottom_color, 7);
	}

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

	DrawCheckerboard(target, brush, size, (size.width / 2) - 7, 7);

	DrawGradient(target, D2D1::ColorF(color.r, color.g, color.b, 1), D2D1::ColorF(color.r, color.g, color.b, 0), 7);

	DrawArrows(target, a, 7);

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