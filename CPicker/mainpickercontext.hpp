#pragma once
#include "rendercontext.hpp"

class MainPickerContext : public RenderContext {
private:
	CComPtr<ID2D1LinearGradientBrush> m_hueGradient;
	CComPtr<ID2D1LinearGradientBrush> m_transparentToBlack;
	CComPtr<ID2D1LinearGradientBrush> m_transparentToWhite;

	HRESULT DrawTwoDimensionalGradient(const D2D1_COLOR_F &top_left, const D2D1_COLOR_F &top_right, const D2D1_COLOR_F &bottom_left, const D2D1_COLOR_F &bottom_right);

protected:
	void ReleaseAll();

public:
	HRESULT Refresh(HWND hwnd);
	HRESULT Draw(const HWND hDlg, const SColourF &col, const SColourF &);
};