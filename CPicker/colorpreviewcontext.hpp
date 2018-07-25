#pragma once
#include "rendercontext.hpp"

class ColorPreviewContext : public RenderContext {
public:
	using RenderContext::RenderContext;
	HRESULT Draw(const HWND, const SColourF &col, const SColourF &old) override;
};