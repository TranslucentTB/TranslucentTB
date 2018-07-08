#pragma once
#include "slidercontext.hpp"

class ColorSliderContext : public SliderContext {
private:
	ComPtr<ID2D1LinearGradientBrush> m_hueGradient;

public:
	inline ColorSliderContext(ID2D1Factory3 *factory) : SliderContext(factory) { }

	HRESULT Refresh(HWND hwnd);
	HRESULT Draw(const HWND hDlg, const SColourF &col, const SColourF &);
};