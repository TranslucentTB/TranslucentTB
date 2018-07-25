#pragma once
#include "slidercontext.hpp"

class ColorSliderContext : public SliderContext {
private:
	ComPtr<ID2D1LinearGradientBrush> m_hueGradient;

public:
	using SliderContext::SliderContext;

	HRESULT Refresh(HWND hwnd) override;
	HRESULT Draw(const HWND hDlg, const SColourF &col, const SColourF &) override;
};