#pragma once
#include <cstdint>
#include <d2d1.h>
#include <d2d1_3.h>

#include "SColour.hpp"

void DrawColorSlider(ID2D1RenderTarget *target, ID2D1SolidColorBrush *brush, ID2D1LinearGradientBrush *hue, const HWND &hDlg, const float &r, const float &g, const float &b, const unsigned short &h, const uint8_t &s, const uint8_t &v);
void DrawAlphaSlider(ID2D1RenderTarget *target, ID2D1SolidColorBrush *brush, const float &r, const float &g, const float &b, const float &a);
void DrawColorIndicator(ID2D1RenderTarget *target, ID2D1SolidColorBrush *brush, const float &rf, const float &gf, const float &bf, const float &af, const SColour &oldcolor, bool flag = true);