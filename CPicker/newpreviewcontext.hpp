#pragma once
#include "previewcontext.hpp"

class NewPreviewContext : public PreviewContext {
private:
	std::wstring_view GetText() override
	{
		return L"New";
	}

public:
	using PreviewContext::PreviewContext;
	inline HRESULT Draw(HWND, const SColourF &col) override
	{
		return DrawPreview(col, false);
	}
};