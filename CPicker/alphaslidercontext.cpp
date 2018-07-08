#include "alphaslidercontext.hpp"

HRESULT AlphaSliderContext::Draw(const HWND, const SColourF &col, const SColourF &)
{
	const DWORD c = GetSysColor(COLOR_BTNFACE);
	const float br = GetRValue(c) / 255.0f;
	const float bg = GetGValue(c) / 255.0f;
	const float bb = GetBValue(c) / 255.0f;

	m_dc->BeginDraw();
	m_dc->Clear(D2D1::ColorF(br, bg, bb));

	m_brush->SetColor(D2D1::ColorF(0, 0.3f));

	const float square_size = m_size.width / 4.0f;

	bool flag = true;
	for (float y = 0.0f; y < m_size.height; y += square_size)
	{
		for (float x = flag ? square_size : square_size * 2.0f; x < m_size.width - square_size; x += square_size * 2.0f)
		{
			m_dc->FillRectangle(
				D2D1::RectF(
					x,
					y,
					x + square_size,
					y + square_size
				),
				m_brush.Get());
		}
		flag = !flag;
	}

	ComPtr<ID2D1LinearGradientBrush> brush;
	HRESULT hr = CreateGradient(&brush, D2D1::ColorF(col.r, col.g, col.b, 1.0f), D2D1::ColorF(col.r, col.g, col.b, 0.0f));
	if (FAILED(hr))
	{
		return hr;
	}

	// https://stackoverflow.com/questions/28900598/how-to-combine-two-colors-with-varying-alpha-values
	const float a = ((1.0f - col.a) * 0.3f) + col.a;
	const float r = (col.a * col.r) / a;
	const float g = (col.a * col.g) / a;
	const float b = (col.a * col.b) / a;

	hr = DrawSlider(1.0f - col.a, D2D1::ColorF(r, g, b, a), brush.Get());
	if (FAILED(hr))
	{
		return hr;
	}

	return EndDraw();
}