#pragma once
#include "slidercontext.hpp"

class ColorSliderContext : public SliderContext {
private:
	CComPtr<ID2D1LinearGradientBrush> m_hueGradient;

protected:
	void ReleaseAll();

public:
	HRESULT Refresh(HWND hwnd);
	HRESULT Draw(const HWND hDlg, const SColourF &col, const SColourF &);
};