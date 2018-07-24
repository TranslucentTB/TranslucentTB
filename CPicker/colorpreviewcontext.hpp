#pragma once
#include "rendercontext.hpp"

class ColorPreviewContext : public RenderContext {
public:
	inline ColorPreviewContext(ID2D1Factory3 *const factory) : RenderContext(factory) { }
	HRESULT Draw(const HWND, const SColourF &col, const SColourF &old) override;
};