#include "drawroutines.hpp"
#include <array>
#include <atlbase.h>

#include "drawhelper.hpp"
#include "resource.h"

constexpr uint8_t HueGradientPrecision = 20; // Number of steps in the hue gradient
const std::array<D2D1_GRADIENT_STOP, HueGradientPrecision> &GetHueGradient();

void DrawColorPicker(ID2D1RenderTarget *target, const HWND &hDlg, const float &r, const float &g, const float &b, const unsigned short &h, const uint8_t &s, const uint8_t &v)
{
	const D2D1_SIZE_F size = target->GetSize();

	target->BeginDraw();

	D2D1_POINT_2F indicator_point;

	// RED
	if (IsDlgButtonChecked(hDlg, IDC_R) == BST_CHECKED)
	{
		DrawTwoDimensionalGradient(target, size, D2D1::ColorF(r, 0.0f, 0.0f), D2D1::ColorF(r, 1.0f, 0.0f), D2D1::ColorF(r, 0.0f, 1.0f), D2D1::ColorF(r, 1.0f, 1.0f));

		indicator_point = D2D1::Point2F(g * size.width, b * size.height);
	}

	// GREEN
	else if (IsDlgButtonChecked(hDlg, IDC_G) == BST_CHECKED)
	{
		DrawTwoDimensionalGradient(target, size, D2D1::ColorF(0.0f, g, 0.0f), D2D1::ColorF(1.0f, g, 0.0f), D2D1::ColorF(0.0f, g, 1.0f), D2D1::ColorF(1.0f, g, 1.0f));

		indicator_point = D2D1::Point2F(r * size.width, b * size.height);
	}

	// BLUE
	else if (IsDlgButtonChecked(hDlg, IDC_B) == BST_CHECKED)
	{
		DrawTwoDimensionalGradient(target, size, D2D1::ColorF(0.0f, 0.0f, b), D2D1::ColorF(0.0f, 1.0f, b), D2D1::ColorF(1.0f, 0.0f, b), D2D1::ColorF(1.0f, 1.0f, b));

		indicator_point = D2D1::Point2F(g * size.width, r * size.height);
	}

	// HUE
	else if (IsDlgButtonChecked(hDlg, IDC_H) == BST_CHECKED)
	{
		SColour temp;
		temp.h = h;
		temp.s = 100;
		temp.v = 100;
		temp.UpdateRGB();
		DrawTwoDimensionalGradient(target, size, D2D1::ColorF(D2D1::ColorF::White), D2D1::ColorF(temp.r / 255.0f, temp.g / 255.0f, temp.b / 255.0f), D2D1::ColorF(D2D1::ColorF::Black), D2D1::ColorF(D2D1::ColorF::Black));

		indicator_point = D2D1::Point2F(s / 100.0f * size.width, (100 - v) / 100.0f * size.height);
	}

	// SATURATION and VALUE
	else if (IsDlgButtonChecked(hDlg, IDC_S) == BST_CHECKED || IsDlgButtonChecked(hDlg, IDC_V) == BST_CHECKED)
	{
		CComPtr<ID2D1GradientStopCollection> pGradientStops;
		target->CreateGradientStopCollection(
			GetHueGradient().data(),
			HueGradientPrecision,
			D2D1_GAMMA_1_0,
			D2D1_EXTEND_MODE_CLAMP,
			&pGradientStops
		);

		CComPtr<ID2D1LinearGradientBrush> brush;
		target->CreateLinearGradientBrush(
			D2D1::LinearGradientBrushProperties(
				D2D1::Point2F(size.width, 0.0f),
				D2D1::Point2F(0.0f, 0.0f)
			),
			pGradientStops,
			&brush
		);

		target->FillRectangle(D2D1::RectF(0.0f, 0.0f, size.width, size.height), brush);

		// SATURATION
		if (IsDlgButtonChecked(hDlg, IDC_S) == BST_CHECKED)
		{
			CComPtr<ID2D1SolidColorBrush> underlayBrush;
			target->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f - (s / 100.0f)), &underlayBrush);

			target->FillRectangle(D2D1::RectF(0.0f, 0.0f, size.width, size.height), underlayBrush);
			DrawGradient(target, size, D2D1::ColorF(0, 0.0f), D2D1::ColorF(D2D1::ColorF::Black));

			indicator_point = D2D1::Point2F(h / 359.0f * size.width, (100 - v) / 100.0f * size.height);
		}

		// VALUE
		else if (IsDlgButtonChecked(hDlg, IDC_V) == BST_CHECKED)
		{
			DrawGradient(target, size, D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.0f), D2D1::ColorF(D2D1::ColorF::White));

			CComPtr<ID2D1SolidColorBrush> overlayBrush;
			target->CreateSolidColorBrush(D2D1::ColorF(0, 1.0f - (v / 100.0f)), &overlayBrush);

			target->FillRectangle(D2D1::RectF(0.0f, 0.0f, size.width, size.height), overlayBrush);

			indicator_point = D2D1::Point2F(h / 359.0f * size.width, (100 - s) / 100.0f * size.height);
		}
	}

	CComPtr<ID2D1SolidColorBrush> brush;
	target->CreateSolidColorBrush(D2D1::ColorF(1.0f - r, 1.0f - g, 1.0f - b), &brush);
	const float circle_radius = size.width / 50.0f;
	target->DrawEllipse(D2D1::Ellipse(indicator_point, circle_radius, circle_radius), brush);

	target->EndDraw();
}

void DrawColorSlider(ID2D1RenderTarget *target, const HWND &hDlg, const float &r, const float &g, const float &b, const unsigned short &h, const uint8_t &s, const uint8_t &v)
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
		CComPtr<ID2D1GradientStopCollection> pGradientStops;
		target->CreateGradientStopCollection(
			GetHueGradient().data(),
			HueGradientPrecision,
			D2D1_GAMMA_1_0,
			D2D1_EXTEND_MODE_CLAMP,
			&pGradientStops
		);

		CComPtr<ID2D1LinearGradientBrush> brush;
		target->CreateLinearGradientBrush(
			D2D1::LinearGradientBrushProperties(
				D2D1::Point2F(0.0f, 0.0f),
				D2D1::Point2F(0.0f, size.height)
			),
			pGradientStops,
			&brush
		);

		target->FillRectangle(D2D1::RectF(border_size, 0.0f, size.width - border_size, size.height), brush);

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

	CComPtr<ID2D1SolidColorBrush> brush;
	target->CreateSolidColorBrush(arrow_color, &brush);
	DrawArrows(target, size, arrow_position, border_size, brush);

	target->EndDraw();
}

void DrawAlphaSlider(ID2D1RenderTarget *target,  const float &r, const float &g, const float &b, const float &a)
{
	const DWORD backgroundColor = GetSysColor(COLOR_BTNFACE);
	const D2D1_SIZE_F size = target->GetSize();

	target->BeginDraw();
	target->Clear(D2D1::ColorF(GetRValue(backgroundColor) / 255.0f, GetGValue(backgroundColor) / 255.0f, GetBValue(backgroundColor) / 255.0f));

	CComPtr<ID2D1SolidColorBrush> brush;
	target->CreateSolidColorBrush(D2D1::ColorF(0, 0.3f), &brush);

	const float border_size = size.width / 4.0f;

	DrawCheckerboard(target, brush, size, border_size, border_size);

	DrawGradient(target, size, D2D1::ColorF(r, g, b, 1.0f), D2D1::ColorF(r, g, b, 0.0f), border_size);

	DrawArrows(target, size, a, border_size, brush);

	CComPtr<ID2D1SolidColorBrush> brush2;
	target->CreateSolidColorBrush(D2D1::ColorF(r, g, b, 1.0f - a), &brush2);
	DrawArrows(target, size, a, border_size, brush2);

	target->EndDraw();
}

void DrawColorIndicator(ID2D1RenderTarget *target, const float &rf, const float &gf, const float &bf, const float &af, const SColour &oldcolor, bool flag)
{
	const D2D1_SIZE_F size = target->GetSize();

	target->BeginDraw();
	target->Clear(D2D1::ColorF(D2D1::ColorF::White));

	CComPtr<ID2D1SolidColorBrush> brush;
	target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &brush);
	DrawCheckerboard(target, brush, size, size.height / 4.0f, 0, flag);
	brush.Release();

	target->CreateSolidColorBrush(D2D1::ColorF(rf, gf, bf, af), &brush);
	target->FillRectangle(D2D1::RectF(0.0f, 0.0f, size.width, size.height / 2.0f), brush);
	brush.Release();

	target->CreateSolidColorBrush(D2D1::ColorF(oldcolor.r / 255.0f, oldcolor.g / 255.0f, oldcolor.b / 255.0f, oldcolor.a / 255.0f), &brush);
	target->FillRectangle(D2D1::RectF(0.0f, size.height / 2.0f, size.width, size.height), brush);

	target->EndDraw();
}

const std::array<D2D1_GRADIENT_STOP, HueGradientPrecision> &GetHueGradient()
{
	static std::array<D2D1_GRADIENT_STOP, HueGradientPrecision> gradientStops;
	static bool calc = false;

	if (!calc)
	{
		SColour tempcol;
		tempcol.h = 359;
		tempcol.s = 100;
		tempcol.v = 100;
		tempcol.UpdateRGB();

		const float step = 359 / (float)HueGradientPrecision;

		for (uint8_t i = 0; i < HueGradientPrecision; i++)
		{
			gradientStops[i].color = D2D1::ColorF(tempcol.r / 255.0f, tempcol.g / 255.0f, tempcol.b / 255.0f);
			gradientStops[i].position = i / (float)(HueGradientPrecision - 1);

			tempcol.h -= step;
			tempcol.UpdateRGB();
		}

		calc = true;
	}

	return gradientStops;
}