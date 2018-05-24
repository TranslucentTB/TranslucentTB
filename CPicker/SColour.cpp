#include "SColour.hpp"
#include <algorithm>

// Updates the RGB color from the HSV
void SColour::UpdateRGB()
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

// Updates the HSV color from the RGB
void SColour::UpdateHSV()
{
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

	short temp;
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