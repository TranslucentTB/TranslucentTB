#include "drawhelper.hpp"
#include <atlbase.h>
#include <d2d1helper.h>

void DrawGradient(ID2D1RenderTarget *target, const D2D1_SIZE_F &size, const D2D1_COLOR_F &top, const D2D1_COLOR_F &bottom, const uint8_t &border_size, const bool &isX)
{
	const D2D1_GRADIENT_STOP gradientStops[] = {
		{
			0.0f,
			top
		},
		{
			1.0f,
			bottom
		}
	};

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
			D2D1::Point2F(0.0f, 0.0f),
			D2D1::Point2F(isX ? size.width : 0.0f, isX ? 0.0f : size.height)
		),
		pGradientStops,
		&brush
	);

	target->FillRectangle(D2D1::RectF(border_size, 0.0f, size.width - border_size, size.height), brush);
}

void DrawArrows(ID2D1RenderTarget *target, const D2D1_SIZE_F &t_size, const float &position, const uint8_t &size, ID2D1Brush *brush)
{
	const D2D1_TRIANGLE triangles[2] = {
		{
			D2D1::Point2F(0.0f, (t_size.height * position) - (size / 2.0f)),
			D2D1::Point2F(0.0f, (t_size.height * position) + (size / 2.0f)),
			D2D1::Point2F(size, t_size.height * position)
		},
		{
			D2D1::Point2F(t_size.width, (t_size.height * position) - (size / 2.0f)),
			D2D1::Point2F(t_size.width, (t_size.height * position) + (size / 2.0f)),
			D2D1::Point2F(t_size.width - size, t_size.height * position)
		}
	};

	CComPtr<ID2D1Mesh> mesh;
	target->CreateMesh(&mesh);

	CComPtr<ID2D1TessellationSink> sink;
	mesh->Open(&sink);

	sink->AddTriangles(triangles, 2);
	sink->Close();

	const D2D1_ANTIALIAS_MODE mode_backup = target->GetAntialiasMode();
	target->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
	target->FillMesh(mesh, brush);
	target->SetAntialiasMode(mode_backup);
}