#include "drawhelper.hpp"
#include <atlbase.h>
#include <d2d1helper.h>

void DrawGradient(ID2D1RenderTarget *target, const D2D1_COLOR_F &top, const D2D1_COLOR_F &bottom, const uint8_t &border_size)
{
	const D2D1_SIZE_F size = target->GetSize();

	D2D1_GRADIENT_STOP gradientStops[2];
	gradientStops[0].color = top;
	gradientStops[0].position = 0.0f;
	gradientStops[1].color = bottom;
	gradientStops[1].position = 1.0f;

	CComPtr<ID2D1GradientStopCollection> pGradientStops;
	target->CreateGradientStopCollection(
		gradientStops,
		2,
		D2D1_GAMMA_1_0,
		D2D1_EXTEND_MODE_CLAMP,
		&pGradientStops
	);

	CComPtr<ID2D1LinearGradientBrush> brush;
	target->CreateLinearGradientBrush(
		D2D1::LinearGradientBrushProperties(
			D2D1::Point2F(0, 0),
			D2D1::Point2F(0, size.height)
		),
		pGradientStops,
		&brush
	);

	target->FillRectangle(D2D1::RectF(border_size, 0, size.width - border_size, size.height), brush);
}

void DrawCheckerboard(ID2D1RenderTarget *target, ID2D1SolidColorBrush *brush, const D2D1_SIZE_F &size, const uint8_t &square_size, const uint8_t &border_size)
{
	bool flag = true;
	for (float y = 0; y < size.height; y += square_size)
	{
		for (float x = flag ? border_size : border_size + square_size; x < size.width - border_size; x += square_size * 2)
		{
			target->FillRectangle(
				D2D1::RectF(
					x,
					y,
					x + square_size,
					y + square_size
				),
				brush);
		}
		flag = !flag;
	}
}