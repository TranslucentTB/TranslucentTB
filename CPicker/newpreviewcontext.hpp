#pragma once
#include "previewcontext.hpp"

class NewPreviewContext : public PreviewContext {
public:
	using PreviewContext::PreviewContext;
	inline HRESULT Draw(HWND, const SColourF &col, const SColourF &) override
	{
		return DrawPreview(col, L"New", false);
	}
};