#pragma once
#include "previewcontext.hpp"

class OldPreviewContext : public PreviewContext {
private:
	SColourF m_old;

	std::wstring_view GetText() override
	{
		return L"Old";
	}

public:
	inline OldPreviewContext(const SColour &old, ID2D1Factory3 *factory, IDWriteFactory *dwFactory) :
		PreviewContext(factory, dwFactory),
		m_old(old)
	{ }

	inline HRESULT Draw(HWND, const SColourF &) override
	{
		return DrawPreview(m_old, true);
	}
};