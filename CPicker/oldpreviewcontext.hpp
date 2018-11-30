#pragma once
#include "previewcontext.hpp"

class OldPreviewContext : public PreviewContext {
public:
	using PreviewContext::PreviewContext;
	inline HRESULT Draw(HWND, const SColourF &, const SColourF &old) override
	{
		return DrawPreview(old, L"Old", true);
	}
};