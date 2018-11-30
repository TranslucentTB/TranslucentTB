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

HRESULT PreviewContext::DrawText(std::wstring_view text)
{
	m_textBmp->BeginDraw();
	m_textBmp->Clear();

	m_textBmp->DrawText(text.data(), text.length(), m_textFormat.get(), D2D1::RectF(0.0f, 0.0f, m_textBmpSize.width, m_textBmpSize.height), m_blackText.get());

	return m_textBmp->EndDraw();
}

HRESULT PreviewContext::DrawColor(const SColourF &col, bool flag)
{
	const DWORD c = GetSysColor(COLOR_BTNFACE);
	const float br = GetRValue(c) / 255.0f;
	const float bg = GetGValue(c) / 255.0f;
	const float bb = GetBValue(c) / 255.0f;

	m_colorBmp->BeginDraw();
	m_colorBmp->Clear(D2D1::ColorF(br, bg, bb));

	m_color->SetColor(D2D1::ColorF(0, 0.3f));

	const float square_size = m_colorBmpSize.height / 4.0f;
	for (float y = 0.0f; y < m_colorBmpSize.height; y += square_size)
	{
		for (float x = flag ? 0 : square_size; x < m_colorBmpSize.width; x += square_size * 2.0f)
		{
			m_colorBmp->FillRectangle(
				D2D1::RectF(
					x,
					y,
					x + square_size,
					y + square_size
				),
				m_color.get());
		}
		flag = !flag;
	}

	m_color->SetColor(D2D1::ColorF(col.r, col.g, col.b, col.a));
	m_colorBmp->FillRectangle(D2D1::RectF(0.0f, 0.0f, m_colorBmpSize.width, m_colorBmpSize.height), m_color.get());

	return m_colorBmp->EndDraw();
}

HRESULT PreviewContext::DrawPreview(const SColourF &col, std::wstring_view text, bool invert)
{
	HRESULT hr = DrawText(text);
	if (FAILED(hr))
	{
		return hr;
	}

	winrt::com_ptr<ID2D1Bitmap> textBmp;
	hr = m_textBmp->GetBitmap(textBmp.put());
	if (FAILED(hr))
	{
		return hr;
	}

	hr = DrawColor(col, invert);
	if (FAILED(hr))
	{
		return hr;
	}

	winrt::com_ptr<ID2D1Bitmap> colorBmp;
	hr = m_colorBmp->GetBitmap(colorBmp.put());
	if (FAILED(hr))
	{
		return hr;
	}

	m_invertEffect->SetInput(0, colorBmp.get());
	m_maskEffect->SetInputEffect(0, m_invertEffect.get());
	m_maskEffect->SetInput(1, textBmp.get());

	DrawContext dc = BeginDraw();
	m_dc->DrawBitmap(colorBmp.get());

	m_dc->DrawImage(m_maskEffect.get());

	return dc.EndDraw();
}

HRESULT PreviewContext::Refresh(HWND hwnd)
{
	HRESULT hr;
	m_textBmp = nullptr;
	m_blackText = nullptr;
	m_colorBmp = nullptr;
	m_color = nullptr;
	m_invertEffect = nullptr;
	m_maskEffect = nullptr;

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

	hr = m_dc->CreateEffect(CLSID_D2D1Invert, m_invertEffect.put());
	if (FAILED(hr))
	{
		return hr;
	}

	hr = m_dc->CreateEffect(CLSID_D2D1AlphaMask, m_maskEffect.put());
	if (FAILED(hr))
	{
		return hr;
	}

	hr = CreateBitmapTarget(m_textBmp, m_blackText, m_textBmpSize);
	if (FAILED(hr))
	{
		return hr;
	}

	hr = CreateBitmapTarget(m_colorBmp, m_color, m_colorBmpSize);
	if (FAILED(hr))
	{
		return hr;
	}

	return S_OK;
}
