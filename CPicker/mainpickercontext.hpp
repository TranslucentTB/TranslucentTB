#pragma once
#include "rendercontext.hpp"

class MainPickerContext : public RenderContext {
private:
	ComPtr<ID2D1LinearGradientBrush> m_hueGradient;
	ComPtr<ID2D1LinearGradientBrush> m_transparentToBlack;
	ComPtr<ID2D1LinearGradientBrush> m_transparentToWhite;

	HRESULT DrawTwoDimensionalGradient(const D2D1_COLOR_F &top_left, const D2D1_COLOR_F &top_right, const D2D1_COLOR_F &bottom_left, const D2D1_COLOR_F &bottom_right);

public:
	inline MainPickerContext(ID2D1Factory3 *factory) : RenderContext(factory) { }

	HRESULT Refresh(HWND hwnd);
	HRESULT Draw(const HWND hDlg, const SColourF &col, const SColourF &);
};