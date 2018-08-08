#include "colorslidercontext.hpp"
#include "huegradient.hpp"
#include "resource.h"

HRESULT ColorSliderContext::Refresh(HWND hwnd)
{
	HRESULT hr;
	m_hueGradient = nullptr;

	hr = SliderContext::Refresh(hwnd);
	if (FAILED(hr))
	{
		return hr;
	}

	hr = CreateHueGradient(m_dc.get(), m_hueGradient.put(), true);
	if (FAILED(hr))
	{
		return hr;
	}

	return S_OK;
}

HRESULT ColorSliderContext::Draw(const HWND hDlg, const SColourF &col, const SColourF &)
{
	winrt::com_ptr<ID2D1LinearGradientBrush> brush;
	const DWORD backgroundColor = GetSysColor(COLOR_BTNFACE);
	DrawContext dc = BeginDraw();
	m_dc->Clear(D2D1::ColorF(GetRValue(backgroundColor) / 255.0f, GetGValue(backgroundColor) / 255.0f, GetBValue(backgroundColor) / 255.0f));

	D2D1_COLOR_F arrow_color = D2D1::ColorF(col.r, col.g, col.b);
	float arrow_position;
	HRESULT hr;

	// Check who is selected.
	// RED
	if (IsDlgButtonChecked(hDlg, IDC_R) == BST_CHECKED)
	{
		hr = CreateGradient(brush.put(), D2D1::ColorF(1.0f, col.g, col.b), D2D1::ColorF(0.0f, col.g, col.b));
		if (FAILED(hr))
		{
			return hr;
		}

		arrow_position = 1.0f - col.r;
	}
	// GREEN
	else if (IsDlgButtonChecked(hDlg, IDC_G) == BST_CHECKED)
	{
		hr = CreateGradient(brush.put(), D2D1::ColorF(col.r, 1.0f, col.b), D2D1::ColorF(col.r, 0.0f, col.b));
		if (FAILED(hr))
		{
			return hr;
		}

		arrow_position = 1.0f - col.g;
	}
	// BLUE
	else if (IsDlgButtonChecked(hDlg, IDC_B) == BST_CHECKED)
	{
		hr = CreateGradient(brush.put(), D2D1::ColorF(col.r, col.g, 1.0f), D2D1::ColorF(col.r, col.g, 0.0f));
		if (FAILED(hr))
		{
			return hr;
		}

		arrow_position = 1.0f - col.b;
	}
	// HUE
	else if (IsDlgButtonChecked(hDlg, IDC_H) == BST_CHECKED)
	{
		brush = m_hueGradient;

		arrow_position = 1.0f - col.h;
		SColour temp;
		temp.h = col.h * 359.0f;
		temp.s = 100;
		temp.v = 100;
		temp.UpdateRGB();
		arrow_color = D2D1::ColorF(temp.r / 255.0f, temp.g / 255.0f, temp.b / 255.0f);
	}
	// SATURATION
	else if (IsDlgButtonChecked(hDlg, IDC_S) == BST_CHECKED)
	{
		SColour tempcol;

		tempcol.h = col.h * 359.0f;
		tempcol.s = 100;
		tempcol.v = col.v * 100.0f;
		tempcol.UpdateRGB();

		const D2D1_COLOR_F top_color = D2D1::ColorF(tempcol.r / 255.0f, tempcol.g / 255.0f, tempcol.b / 255.0f);

		tempcol.s = 0;
		tempcol.UpdateRGB();
		const D2D1_COLOR_F bottom_color = D2D1::ColorF(tempcol.r / 255.0f, tempcol.g / 255.0f, tempcol.b / 255.0f);

		hr = CreateGradient(brush.put(), top_color, bottom_color);
		if (FAILED(hr))
		{
			return hr;
		}

		arrow_position = 1.0f - col.s;
	}
	// VALUE
	else if (IsDlgButtonChecked(hDlg, IDC_V) == BST_CHECKED)
	{
		SColour tempcol;

		tempcol.h = col.h * 359.0f;
		tempcol.s = col.s * 100.0f;
		tempcol.v = 100;
		tempcol.UpdateRGB();

		const D2D1_COLOR_F top_color = D2D1::ColorF(tempcol.r / 255.0f, tempcol.g / 255.0f, tempcol.b / 255.0f);

		tempcol.v = 0;
		tempcol.UpdateRGB();
		const D2D1_COLOR_F bottom_color = D2D1::ColorF(tempcol.r / 255.0f, tempcol.g / 255.0f, tempcol.b / 255.0f);

		hr = CreateGradient(brush.put(), top_color, bottom_color);
		if (FAILED(hr))
		{
			return hr;
		}

		arrow_position = 1.0f - col.v;
	}

	hr = DrawSlider(arrow_position, arrow_color, brush.get());
	if (FAILED(hr))
	{
		return hr;
	}

	return dc.EndDraw();
}