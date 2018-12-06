#include "previewcontext.hpp"

HRESULT PreviewContext::DrawPreview(const SColourF &col, bool invert)
{
	const auto text = GetText();
	DrawContext dc = BeginDraw();

	m_dc->Clear(D2D1::ColorF(0, 0.0f));

	const float square_size = m_size.height / 4.0f;
	for (float y = 0.0f; y < m_size.height; y += square_size)
	{
		for (float x = invert ? 0 : square_size; x < m_size.width; x += square_size * 2.0f)
		{
			m_dc->FillRectangle(
				D2D1::RectF(
					x,
					y,
					x + square_size,
					y + square_size
				),
				m_checkerboard.get());
		}
		invert = !invert;
	}

	m_brush->SetColor(D2D1::ColorF(col.r, col.g, col.b, col.a));
	m_dc->FillRectangle(D2D1::RectF(0.0f, 0.0f, m_size.width, m_size.height), m_brush.get());

	m_brush->SetColor(D2D1::ColorF(1.0f - col.r, 1.0f - col.g, 1.0f - col.b));
	m_dc->DrawText(
		text.data(),
		text.length(),
		m_textFormat.get(),
		D2D1::RectF(0.0f, 0.0f, m_size.width, m_size.height),
		m_brush.get()
	);

	return dc.EndDraw();
}

void PreviewContext::Destroy()
{
	m_checkerboard = nullptr;

	RenderContext::Destroy();
}

HRESULT PreviewContext::Refresh(HWND hwnd)
{
	HRESULT hr;

	if (!m_textFormat)
	{
		hr = m_factory->CreateTextFormat(
			L"MS Shell Dlg",
			NULL,
			DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			20.0,
			L"en-US",
			m_textFormat.put()
		);
		if (FAILED(hr))
		{
			return hr;
		}

		hr = m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
		if (FAILED(hr))
		{
			return hr;
		}

		m_textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
		if (FAILED(hr))
		{
			return hr;
		}
	}

	hr = RenderContext::Refresh(hwnd);
	if (FAILED(hr))
	{
		return hr;
	}

	hr = m_dc->CreateSolidColorBrush(D2D1::ColorF(0, 0.3f), m_checkerboard.put());
	if (FAILED(hr))
	{
		return hr;
	}

	return S_OK;
}
