#pragma once
#include <cstdint>
#include <d2d1.h>

void DrawGradient(ID2D1RenderTarget *target, const D2D1_COLOR_F &top, const D2D1_COLOR_F &bottom, const uint8_t &border_size);
void DrawCheckerboard(ID2D1RenderTarget *target, ID2D1SolidColorBrush *brush, const D2D1_SIZE_F &size, const uint8_t &square_size, const uint8_t &border_size = 0);