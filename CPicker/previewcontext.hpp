#pragma once
#include "rendercontext.hpp"
#include <string_view>
#include <dwrite.h>

class PreviewContext : public RenderContext {
private:
	IDWriteFactory *m_factory;
	winrt::com_ptr<IDWriteTextFormat> m_textFormat;
	winrt::com_ptr<ID2D1SolidColorBrush> m_checkerboard;

	HRESULT DrawText(std::wstring_view text, const SColourF &col);
	HRESULT DrawColor(const SColourF &col, bool flag);

protected:
	HRESULT DrawPreview(const SColourF &col, bool invert);
	virtual std::wstring_view GetText() = 0;

	void Destroy() override;

public:
	inline PreviewContext(ID2D1Factory3 *factory, IDWriteFactory *dwFactory) :
		RenderContext(factory),
		m_factory(dwFactory)
	{ }

	HRESULT Refresh(HWND hwnd) override;
};