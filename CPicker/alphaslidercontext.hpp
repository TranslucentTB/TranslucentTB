#pragma once
#include "slidercontext.hpp"

class AlphaSliderContext : public SliderContext {
public:
	inline AlphaSliderContext(ID2D1Factory3 *const factory) : SliderContext(factory) { }
	HRESULT Draw(const HWND, const SColourF &col, const SColourF &);
};