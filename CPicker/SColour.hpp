#pragma once
#include <cstdint>

struct _declspec(dllexport) SColour
{
	// Red, green and blue
	uint8_t r, g, b;

	// Hue, saturation and value
	unsigned short h;
	uint8_t s, v;

	// Alpha
	uint8_t a;

	void UpdateRGB();			// Updates RGB from HSV
	void UpdateHSV();			// Updates HSV from RGB
};