#pragma once
#include "rendercontext.hpp"
#include <string_view>
#include <dwrite.h>

class PreviewContext : public RenderContext {
private:
	IDWriteFactory *m_factory;
	winrt::com_ptr<IDWriteTextFormat> m_textFormat;

	winrt::com_ptr<ID2D1BitmapRenderTarget> m_bmpTarget;
	winrt::com_ptr<ID2D1SolidColorBrush> m_color;
	winrt::com_ptr<ID2D1SolidColorBrush> m_checkerboard;
	winrt::com_ptr<ID2D1Bitmap> m_bmp;

	winrt::com_ptr<ID2D1Effect> m_effect;

	D2D1_SIZE_F m_bmpSize;

	HRESULT CreateBitmapTarget(winrt::com_ptr<ID2D1BitmapRenderTarget> &target, winrt::com_ptr<ID2D1SolidColorBrush> &brush, D2D1_SIZE_F &size);
	HRESULT CreateTextBitmap(ID2D1Bitmap **out);

	HRESULT DrawColor(const SColourF &col, bool flag);

protected:
	HRESULT DrawPreview(const SColourF &col, bool invert);
	virtual std::wstring_view GetText() = 0;

	void Destroy() override;

public:
	inline PreviewContext(ID2D1Factory3 *factory, IDWriteFactory *dwFactory) :
		RenderContext(factory),
		m_factory(dwFactory),
		m_bmpSize()
	{ }

	HRESULT Refresh(HWND hwnd) override;
};