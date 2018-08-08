#include "slidercontext.hpp"

HRESULT SliderContext::DrawSlider(const float &arrow_position, const D2D1_COLOR_F &arrow_color, ID2D1LinearGradientBrush *gradient_brush)
{
	const float border_size = m_size.width / 4.0f;

	m_dc->FillRectangle(D2D1::RectF(border_size, 0.0f, m_size.width - border_size, m_size.height), gradient_brush);

	const D2D1_TRIANGLE triangles[2] = {
		{
			D2D1::Point2F(0.0f, (m_size.height * arrow_position) - (border_size / 2.0f)),
			D2D1::Point2F(0.0f, (m_size.height * arrow_position) + (border_size / 2.0f)),
			D2D1::Point2F(border_size, m_size.height * arrow_position)
		},
		{
			D2D1::Point2F(m_size.width, (m_size.height * arrow_position) - (border_size / 2.0f)),
			D2D1::Point2F(m_size.width, (m_size.height * arrow_position) + (border_size / 2.0f)),
			D2D1::Point2F(m_size.width - border_size, m_size.height * arrow_position)
		}
	};

	winrt::com_ptr<ID2D1Mesh> mesh;
	HRESULT hr = m_dc->CreateMesh(mesh.put());
	if (FAILED(hr))
	{
		return hr;
	}

	winrt::com_ptr<ID2D1TessellationSink> sink;
	hr = mesh->Open(sink.put());
	if (FAILED(hr))
	{
		return hr;
	}

	sink->AddTriangles(triangles, 2);
	hr = sink->Close();
	if (FAILED(hr))
	{
		return hr;
	}

	const D2D1_ANTIALIAS_MODE mode_backup = m_dc->GetAntialiasMode();
	m_dc->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
	m_brush->SetColor(arrow_color);
	m_dc->FillMesh(mesh.get(), m_brush.get());
	m_dc->SetAntialiasMode(mode_backup);

	return S_OK;
}