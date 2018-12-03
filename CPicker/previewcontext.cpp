#include "previewcontext.hpp"

HRESULT PreviewContext::CreateBitmapTarget(winrt::com_ptr<ID2D1BitmapRenderTarget> &target, winrt::com_ptr<ID2D1SolidColorBrush> &brush, D2D1_SIZE_F &size)
{
	HRESULT hr;

	hr = m_dc->CreateCompatibleRenderTarget(target.put());
	if (FAILED(hr))
	{
		return hr;
	}

	size = target->GetSize();

	hr = target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), brush.put());
	if (FAILED(hr))
	{
		return hr;
	}

	return S_OK;
}

HRESULT PreviewContext::CreateTextBitmap(ID2D1Bitmap **out)
{
	winrt::com_ptr<ID2D1BitmapRenderTarget> target;
	winrt::com_ptr<ID2D1SolidColorBrush> brush;
	D2D1_SIZE_F size;

	HRESULT hr = CreateBitmapTarget(target, brush, size);
	if (FAILED(hr))
	{
		return hr;
	}

	auto text = GetText();

	target->BeginDraw();
	target->Clear();

	target->DrawText(text.data(), text.length(), m_textFormat.get(), D2D1::RectF(0.0f, 0.0f, size.width, size.height), brush.get());

	hr = target->EndDraw();
	if (FAILED(hr))
	{
		return hr;
	}

	return target->GetBitmap(out);
}

HRESULT PreviewContext::DrawColor(const SColourF &col, bool flag)
{
	const DWORD c = GetSysColor(COLOR_BTNFACE);
	const float br = GetRValue(c) / 255.0f;
	const float bg = GetGValue(c) / 255.0f;
	const float bb = GetBValue(c) / 255.0f;

	m_bmpTarget->BeginDraw();
	m_bmpTarget->Clear(D2D1::ColorF(br, bg, bb));

	const float square_size = m_bmpSize.height / 4.0f;
	for (float y = 0.0f; y < m_bmpSize.height; y += square_size)
	{
		for (float x = flag ? 0 : square_size; x < m_bmpSize.width; x += square_size * 2.0f)
		{
			m_bmpTarget->FillRectangle(
				D2D1::RectF(
					x,
					y,
					x + square_size,
					y + square_size
				),
				m_checkerboard.get());
		}
		flag = !flag;
	}

	m_color->SetColor(D2D1::ColorF(col.r, col.g, col.b, col.a));
	m_bmpTarget->FillRectangle(D2D1::RectF(0.0f, 0.0f, m_bmpSize.width, m_bmpSize.height), m_color.get());

	return m_bmpTarget->EndDraw();
}

HRESULT PreviewContext::DrawPreview(const SColourF &col, bool invert)
{
	HRESULT hr = DrawColor(col, invert);
	if (FAILED(hr))
	{
		return hr;
	}

	DrawContext dc = BeginDraw();
	m_dc->DrawBitmap(m_bmp.get());

	m_dc->DrawImage(m_effect.get());

	return dc.EndDraw();
}

HRESULT PreviewContext::Refresh(HWND hwnd)
{
	HRESULT hr;
	m_bmp = nullptr;
	m_color = nullptr;
	m_checkerboard = nullptr;
	m_effect = nullptr;
	m_bmpTarget = nullptr;

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

	hr = CreateBitmapTarget(m_bmpTarget, m_color, m_bmpSize);
	if (FAILED(hr))
	{
		return hr;
	}

	hr = m_bmpTarget->CreateSolidColorBrush(D2D1::ColorF(0, 0.3f), m_checkerboard.put());
	if (FAILED(hr))
	{
		return hr;
	}

	hr = m_bmpTarget->GetBitmap(m_bmp.put());
	if (FAILED(hr))
	{
		return hr;
	}

	winrt::com_ptr<ID2D1Bitmap> textBitmap;
	hr = CreateTextBitmap(textBitmap.put());
	if (FAILED(hr))
	{
		return hr;
	}

	winrt::com_ptr<ID2D1Effect> invertEffect;
	hr = m_dc->CreateEffect(CLSID_D2D1Invert, invertEffect.put());
	if (FAILED(hr))
	{
		return hr;
	}

	invertEffect->SetInput(0, m_bmp.get());

	hr = m_dc->CreateEffect(CLSID_D2D1AlphaMask, m_effect.put());
	if (FAILED(hr))
	{
		return hr;
	}

	m_effect->SetInputEffect(0, invertEffect.get());
	m_effect->SetInput(1, textBitmap.get());

	return S_OK;
}
