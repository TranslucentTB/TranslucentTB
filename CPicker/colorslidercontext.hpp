#pragma once
#include "slidercontext.hpp"

class ColorSliderContext : public SliderContext {
private:
	ComPtr<ID2D1LinearGradientBrush> m_hueGradient;

public:
	inline ColorSliderContext(ID2D1Factory3 *const factory) : SliderContext(factory) { }

	HRESULT Refresh(HWND hwnd) override;
	HRESULT Draw(const HWND hDlg, const SColourF &col, const SColourF &) override;
};