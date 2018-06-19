#pragma once
#include "../TranslucentTB/arch.h"
#include <atlbase.h>
#include <d2d1.h>

#include "CColourPicker.hpp"

struct PickerData {
	CColourPicker *picker;
	bool changing_text;
	bool changing_hex_via_spin;
	HWND old_color_tip;
	WNDPROC button_proc;
	CComPtr<ID2D1Factory> factory;

	// Main picker
	CComPtr<ID2D1HwndRenderTarget> targetC1;
	CComPtr<ID2D1SolidColorBrush> brushC1;
	CComPtr<ID2D1LinearGradientBrush> hueC1;
	CComPtr<ID2D1LinearGradientBrush> transparentToBlackC1;
	CComPtr<ID2D1LinearGradientBrush> transparentToWhiteC1;

	// Color slider
	CComPtr<ID2D1HwndRenderTarget> targetC2;
	CComPtr<ID2D1SolidColorBrush> brushC2;
	CComPtr<ID2D1LinearGradientBrush> hueC2;

	// Alpha slider
	CComPtr<ID2D1HwndRenderTarget> targetA;
	CComPtr<ID2D1SolidColorBrush> brushA;

	// Old/new indicator
	CComPtr<ID2D1HwndRenderTarget> targetC;
	CComPtr<ID2D1SolidColorBrush> brushC;
};