#pragma once
#include "slidercontext.hpp"

class AlphaSliderContext : public SliderContext {
public:
	using SliderContext::SliderContext;
	HRESULT Draw(const HWND, const SColourF &col, const SColourF &) override;
};