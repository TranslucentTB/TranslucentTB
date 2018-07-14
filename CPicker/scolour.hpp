#pragma once
#include <algorithm>
#include <cstdint>

struct SColour {

	// Red, green and blue
	uint8_t r, g, b;

	// Hue, saturation and value
	uint16_t h;
	uint8_t s, v;

	// Alpha
	uint8_t a;

#pragma warning(push)
#pragma warning(disable: 4244)
	// Updates RGB from HSV
	constexpr void UpdateRGB()
	{
		const float val = v / 100.0f;

		if (s == 0) // Acromatic color (gray). Hue doesn't mind.
		{
			r = b = g = 255.0f * val;
			return;
		}

		const float base = 255.0f * (1.0f - (s / 100.0f)) * val;

		switch (h / 60)
		{
		case 0:
			r = 255.0f * val;
			g = (255.0f * val - base) * (h / 60.0f) + base;
			b = base;
			break;

		case 1:
			r = (255.0f * val - base) * (1.0f - ((h % 60) / 60.0f)) + base;
			g = 255.0f * val;
			b = base;
			break;

		case 2:
			r = base;
			g = 255.0f * val;
			b = (255.0f * val - base) * ((h % 60) / 60.0f) + base;
			break;

		case 3:
			r = base;
			g = (255.0f * val - base) * (1.0f - ((h % 60) / 60.0f)) + base;
			b = 255.0f * val;
			break;

		case 4:
			r = (255.0f * val - base) * ((h % 60) / 60.0f) + base;
			g = base;
			b = 255.0f * val;
			break;

		case 5:
			r = 255.0f * val;
			g = base;
			b = (255.0f * val - base) * (1.0f - ((h % 60) / 60.0f)) + base;
			break;
		}
	}

	// Updates HSV from RGB
	constexpr void UpdateHSV()
	{
		// Due to Clang headers on MSBuild, can't use constexpr std::max(std::initializer_list)
		const uint8_t &max = (std::max)((std::max)(r, g), b);
		const uint8_t &min = (std::min)((std::min)(r, g), b);
		const float delta = max - min;

		if (max == 0)
		{
			s = h = v = 0;
			return;
		}

		v = max / 255.0f * 100.0f;
		s = delta / (float)max * 100.0f;

		short temp = 0;
		if (r == max)
		{
			temp = 60 * ((g - b) / delta);
		}
		else if (g == max)
		{
			temp = 60 * (2 + (b - r) / delta);
		}
		else
		{
			temp = 60 * (4 + (r - g) / delta);
		}

		if (temp < 0)
		{
			h = temp + 360;
		}
		else
		{
			h = temp;
		}
	}
#pragma warning(pop)
};

// Used for Direct2D rendering
struct SColourF {
	float r, g, b, a, h, s, v;

	constexpr SColourF(const SColour &col) :
		r(col.r / 255.0f),
		g(col.g / 255.0f),
		b(col.b / 255.0f),
		a(col.a / 255.0f),
		h(col.h / 359.0f),
		s(col.s / 100.0f),
		v(col.v / 100.0f)
	{ }
};