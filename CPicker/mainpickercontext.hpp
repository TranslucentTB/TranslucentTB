#pragma once
#include "rendercontext.hpp"

class MainPickerContext : public RenderContext {
private:
	winrt::com_ptr<ID2D1LinearGradientBrush> m_hueGradient;
	winrt::com_ptr<ID2D1LinearGradientBrush> m_transparentToBlack;
	winrt::com_ptr<ID2D1LinearGradientBrush> m_transparentToWhite;

	HRESULT DrawTwoDimensionalGradient(const D2D1_COLOR_F &top_left, const D2D1_COLOR_F &top_right, const D2D1_COLOR_F &bottom_left, const D2D1_COLOR_F &bottom_right);

public:
	using RenderContext::RenderContext;

	HRESULT Refresh(HWND hwnd) override;
	HRESULT Draw(const HWND hDlg, const SColourF &col, const SColourF &) override;
};