#pragma once
#include "slidercontext.hpp"

class AlphaSliderContext : public SliderContext {
public:
	HRESULT Draw(const HWND, const SColourF &col, const SColourF &);
};