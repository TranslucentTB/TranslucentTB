#pragma once
#include "previewcontext.hpp"

class OldPreviewContext : public PreviewContext {
private:
	std::wstring_view GetText() override
	{
		return L"Old";
	}

public:
	using PreviewContext::PreviewContext;
	inline HRESULT Draw(HWND, const SColourF &, const SColourF &old) override
	{
		return DrawPreview(old, true);
	}
};