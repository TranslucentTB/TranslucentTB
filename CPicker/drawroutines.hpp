#pragma once
#include <cstdint>
#include <d2d1.h>
#include <d2d1_3.h>

#include "scolour.hpp"

void DrawColorSlider(ID2D1RenderTarget *target, ID2D1SolidColorBrush *brush, ID2D1LinearGradientBrush *hue, const HWND &hDlg, const float &r, const float &g, const float &b, const unsigned short &h, const uint8_t &s, const uint8_t &v);