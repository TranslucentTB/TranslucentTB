#include "colorpreviewcontext.hpp"

HRESULT ColorPreviewContext::Draw(const HWND, const SColourF &col, const SColourF &old)
{
	m_dc->BeginDraw();

	m_dc->Clear(D2D1::ColorF(D2D1::ColorF::White));

	m_brush->SetColor(D2D1::ColorF(D2D1::ColorF::Black));
	bool flag = true;
	const float square_size = m_size.height / 4.0f;
	for (float y = 0.0f; y < m_size.height; y += square_size)
	{
		for (float x = flag ? 0 : square_size; x < m_size.width; x += square_size * 2.0f)
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

	m_brush->SetColor(D2D1::ColorF(col.r, col.g, col.b, col.a));
	m_dc->FillRectangle(D2D1::RectF(0.0f, 0.0f, m_size.width, m_size.height / 2.0f), m_brush.Get());

	m_brush->SetColor(D2D1::ColorF(old.r, old.g, old.b, old.a));
	m_dc->FillRectangle(D2D1::RectF(0.0f, m_size.height / 2.0f, m_size.width, m_size.height), m_brush.Get());

	return EndDraw();
}