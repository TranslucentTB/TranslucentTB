#include "SColour.hpp"
#include <algorithm>

// Updates the RGB color from the HSV
void SColour::UpdateRGB()
{
	int conv;
	double hue, sat, val;
	int base;

	hue = (float)h / 100.0f;
	sat = (float)s / 100.0f;
	val = (float)v / 100.0f;

	if ((float)s == 0) // Acromatic color (gray). Hue doesn't mind.
	{
		conv = (unsigned short)(255.0f * val);
		r = b = g = conv;
		return;
	}

	base = (unsigned short)(255.0f * (1.0 - sat) * val);

	switch ((unsigned short)((float)h / 60.0f))
	{
	case 0:
		r = (unsigned short)(255.0f * val);
		g = (unsigned short)((255.0f * val - base) * (h / 60.0f) + base);
		b = base;
		break;

	case 1:
		r = (unsigned short)((255.0f * val - base) * (1.0f - ((h % 60) / 60.0f)) + base);
		g = (unsigned short)(255.0f * val);
		b = base;
		break;

	case 2:
		r = base;
		g = (unsigned short)(255.0f * val);
		b = (unsigned short)((255.0f * val - base) * ((h % 60) / 60.0f) + base);
		break;

	case 3:
		r = base;
		g = (unsigned short)((255.0f * val - base) * (1.0f - ((h % 60) / 60.0f)) + base);
		b = (unsigned short)(255.0f * val);
		break;

	case 4:
		r = (unsigned short)((255.0f * val - base) * ((h % 60) / 60.0f) + base);
		g = base;
		b = (unsigned short)(255.0f * val);
		break;

	case 5:
		r = (unsigned short)(255.0f * val);
		g = base;
		b = (unsigned short)((255.0f * val - base) * (1.0f - ((h % 60) / 60.0f)) + base);
		break;
	}
}

// Updates the HSV color from the RGB
void SColour::UpdateHSV()
{
	const uint8_t &max = (std::max)((std::max)(r, g), b);
	const uint8_t &min = (std::min)((std::min)(r, g), b);
	const uint8_t delta = max - min;

	short temp;

	if (max == 0)
	{
		s = h = v = 0;
		return;
	}

	v = max / 255.0 * 100.0;
	s = ((double)delta / max) * 100.0;

	if (r == max)
		temp = 60 * ((g - b) / (double)delta);
	else if (g == max)
		temp = 60 * (2.0 + (b - r) / (double)delta);
	else
		temp = 60 * (4.0 + (r - g) / (double)delta);

	if (temp < 0)
		h = temp + 360;
	else
		h = temp;
}