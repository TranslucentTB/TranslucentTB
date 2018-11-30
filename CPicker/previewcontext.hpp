#pragma once
#include "rendercontext.hpp"
#include <string_view>
#include <dwrite.h>

class PreviewContext : public RenderContext {
private:
	IDWriteFactory *m_factory;

	winrt::com_ptr<ID2D1BitmapRenderTarget> m_textBmp;
	winrt::com_ptr<ID2D1SolidColorBrush> m_blackText;
	winrt::com_ptr<IDWriteTextFormat> m_textFormat;

	winrt::com_ptr<ID2D1BitmapRenderTarget> m_colorBmp;
	winrt::com_ptr<ID2D1SolidColorBrush> m_color;

	D2D1_SIZE_F m_textBmpSize;
	D2D1_SIZE_F m_colorBmpSize;

	winrt::com_ptr<ID2D1Effect> m_invertEffect;
	winrt::com_ptr<ID2D1Effect> m_maskEffect;

	HRESULT CreateBitmapTarget(winrt::com_ptr<ID2D1BitmapRenderTarget> &target, winrt::com_ptr<ID2D1SolidColorBrush> &brush, D2D1_SIZE_F &size);

	HRESULT DrawText(std::wstring_view text);
	HRESULT DrawColor(const SColourF &col, bool flag);

protected:
	HRESULT DrawPreview(const SColourF &col, std::wstring_view text, bool invert);

public:
	inline PreviewContext(ID2D1Factory3 *factory, IDWriteFactory *dwFactory) :
		RenderContext(factory),
		m_factory(dwFactory),
		m_textBmpSize(),
		m_colorBmpSize()
	{ }

	HRESULT Refresh(HWND hwnd) override;
};