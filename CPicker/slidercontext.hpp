#pragma once
#include "rendercontext.hpp"

class SliderContext : public RenderContext {
protected:
	inline SliderContext(ID2D1Factory3 *const factory) : RenderContext(factory) { }
	HRESULT DrawSlider(const float &arrow_position, const D2D1_COLOR_F &arrow_color, ID2D1LinearGradientBrush *gradient_brush);
};