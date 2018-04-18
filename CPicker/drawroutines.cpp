#include "drawroutines.hpp"
#include <array>
#include <atlbase.h>

#include "drawhelper.hpp"
#include "resource.h"

constexpr uint8_t HueGradientPrecision = 20; // Number of steps in the hue gradient
const std::array<D2D1_GRADIENT_STOP, HueGradientPrecision> &GetHueGradient();

constexpr uint8_t BorderSize = 7;

void DrawColorPicker(ID2D1RenderTarget *target, const HWND &hDlg, const float &r, const float &g, const float &b, const unsigned short &h, const uint8_t &s, const uint8_t &v)
{
	target->BeginDraw();

	const D2D1_SIZE_F size = target->GetSize();

	// TODO: Red, green and blue
	// HUE
	if (IsDlgButtonChecked(hDlg, IDC_H) == BST_CHECKED)
	{
		SColour temp;
		temp.h = h;
		temp.s = 100;
		temp.v = 100;
		temp.UpdateRGB();
		target->Clear(D2D1::ColorF(temp.r / 255.0f, temp.g / 255.0f, temp.b / 255.0f));

		DrawGradient(target, D2D1::ColorF(D2D1::ColorF::White), D2D1::ColorF(1, 1, 1, 0), 0, true);
		DrawGradient(target, D2D1::ColorF(0, 0), D2D1::ColorF(D2D1::ColorF::Black), 0, false);
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
				D2D1::Point2F(size.width, 0),
				D2D1::Point2F(0, 0)
			),
			pGradientStops,
			&brush
		);

		target->FillRectangle(D2D1::RectF(0, 0, size.width, size.height), brush);

		// VALUE
		if (IsDlgButtonChecked(hDlg, IDC_V) == BST_CHECKED)
		{
			DrawGradient(target, D2D1::ColorF(1, 1, 1, 0), D2D1::ColorF(D2D1::ColorF::White));

			CComPtr<ID2D1SolidColorBrush> overlayBrush;
			target->CreateSolidColorBrush(D2D1::ColorF(0, 1.0f - (v / 100.0f)), &overlayBrush);

			target->FillRectangle(D2D1::RectF(0, 0, size.width, size.height), overlayBrush);
		}

		// SATURATION
		else
		{
			CComPtr<ID2D1SolidColorBrush> underlayBrush;
			target->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1, 1.0f - (s / 100.0f)), &underlayBrush);

			target->FillRectangle(D2D1::RectF(0, 0, size.width, size.height), underlayBrush);
			DrawGradient(target, D2D1::ColorF(0, 0), D2D1::ColorF(D2D1::ColorF::Black));
		}
	}

	target->EndDraw();
}

void DrawColorSlider(ID2D1RenderTarget *target, const HWND &hDlg, const float &r, const float &g, const float &b, const unsigned short &h, const uint8_t &s, const uint8_t &v)
{
	const DWORD backgroundColor = GetSysColor(COLOR_BTNFACE);

	target->BeginDraw();
	target->Clear(D2D1::ColorF(GetRValue(backgroundColor) / 255.0f, GetGValue(backgroundColor) / 255.0f, GetBValue(backgroundColor) / 255.0f));

	D2D1_COLOR_F top_color, bottom_color, arrow_color;
	float arrow_position;

	// Check who is selected.
	// RED
	if (IsDlgButtonChecked(hDlg, IDC_R) == BST_CHECKED)
	{
		top_color = D2D1::ColorF(1, g, b);
		bottom_color = D2D1::ColorF(0, g, b);

		arrow_position = 1.0 - r;
		arrow_color = D2D1::ColorF(r, g, b);
	}
	// GREEN
	else if (IsDlgButtonChecked(hDlg, IDC_G) == BST_CHECKED)
	{
		top_color = D2D1::ColorF(r, 1, b);
		bottom_color = D2D1::ColorF(r, 0, b);

		arrow_position = 1.0 - g;
		arrow_color = D2D1::ColorF(r, g, b);
	}
	// BLUE
	else if (IsDlgButtonChecked(hDlg, IDC_B) == BST_CHECKED)
	{
		top_color = D2D1::ColorF(r, g, 1);
		bottom_color = D2D1::ColorF(r, g, 0);

		arrow_position = 1.0 - b;
		arrow_color = D2D1::ColorF(r, g, b);
	}
	// HUE
	else if (IsDlgButtonChecked(hDlg, IDC_H) == BST_CHECKED)
	{
		const D2D1_SIZE_F size = target->GetSize();		

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
				D2D1::Point2F(0, 0),
				D2D1::Point2F(0, size.height)
			),
			pGradientStops,
			&brush
		);

		target->FillRectangle(D2D1::RectF(BorderSize, 0, size.width - BorderSize, size.height), brush);

		arrow_position = 1.0 - (h / 359.0f);
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

		arrow_position = 1.0 - (s / 100.0f);
		arrow_color = D2D1::ColorF(r, g, b);
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

		arrow_position = 1.0 - (v / 100.0f);
		arrow_color = D2D1::ColorF(r, g, b);
	}

	// Hue has its own drawing code (a gradient with 20 steps), because it loops over the whole color space.
	// Meaning that if we use top_color and bottom_color it'll just appear as a big single-colored slider.
	if (IsDlgButtonChecked(hDlg, IDC_H) != BST_CHECKED)
	{
		DrawGradient(target, top_color, bottom_color, BorderSize);
	}

	CComPtr<ID2D1SolidColorBrush> brush;
	target->CreateSolidColorBrush(arrow_color, &brush);
	DrawArrows(target, arrow_position, BorderSize, brush);

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

	DrawCheckerboard(target, brush, size, (size.width / 2) - BorderSize, BorderSize);

	DrawGradient(target, D2D1::ColorF(color.r, color.g, color.b, 1), D2D1::ColorF(color.r, color.g, color.b, 0), BorderSize);

	DrawArrows(target, a, BorderSize, brush);

	CComPtr<ID2D1SolidColorBrush> brush2;
	target->CreateSolidColorBrush(D2D1::ColorF(color.r, color.g, color.b, 1.0f - a), &brush2);
	DrawArrows(target, a, BorderSize, brush2);

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
	}

	return gradientStops;
}
