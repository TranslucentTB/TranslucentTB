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

void DrawTwoDimensionalGradient(ID2D1DeviceContext2 *context, const D2D1_SIZE_F &size, const D2D1_COLOR_F &top_left, const D2D1_COLOR_F &top_right, const D2D1_COLOR_F &bottom_left, const D2D1_COLOR_F &bottom_right)
{
	// IDK man.
	const D2D1_GRADIENT_MESH_PATCH meshPatch = D2D1::GradientMeshPatch(
		D2D1::Point2F(),
		D2D1::Point2F(size.width / 2.0f),
		D2D1::Point2F(size.width / 2.0f),
		D2D1::Point2F(size.width),
		D2D1::Point2F(0.0f, size.height / 2.0f),
		D2D1::Point2F(size.width / 3.0f, size.height / 3.0f),
		D2D1::Point2F(size.width / 3.0f * 2.0f, size.height / 3.0f),
		D2D1::Point2F(size.width, size.height / 2.0f),
		D2D1::Point2F(0.0f, size.height / 2.0f),
		D2D1::Point2F(size.width / 3.0f, size.height / 3.0f * 2.0f),
		D2D1::Point2F(size.width / 3.0f * 2.0f, size.height / 3.0f * 2.0f),
		D2D1::Point2F(size.width, size.height / 2.0f),
		D2D1::Point2F(0.0f, size.height),
		D2D1::Point2F(size.width / 2.0f, size.height),
		D2D1::Point2F(size.width / 2.0f, size.height),
		D2D1::Point2F(size.width, size.height),
		top_left,
		top_right,
		bottom_left,
		bottom_right,
		D2D1_PATCH_EDGE_MODE_ANTIALIASED,
		D2D1_PATCH_EDGE_MODE_ANTIALIASED,
		D2D1_PATCH_EDGE_MODE_ANTIALIASED,
		D2D1_PATCH_EDGE_MODE_ANTIALIASED
	);

	CComPtr<ID2D1GradientMesh> mesh;
	context->CreateGradientMesh(&meshPatch, 1, &mesh);
	context->DrawGradientMesh(mesh);
}

void DrawCheckerboard(ID2D1RenderTarget *target, ID2D1SolidColorBrush *brush, const D2D1_SIZE_F &size, const float &square_size, const float &border_size, bool flag)
{
	for (float y = 0.0f; y < size.height; y += square_size)
	{
		for (float x = flag ? border_size : border_size + square_size; x < size.width - border_size; x += square_size * 2.0f)
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