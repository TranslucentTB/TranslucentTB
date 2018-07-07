#pragma once
#include "rendercontext.hpp"

class ColorPreviewContext : public RenderContext {
public:
	HRESULT Draw(const HWND, const SColourF &col, const SColourF &old);
};