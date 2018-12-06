#include "mainpickercontext.hpp"
#include "huegradient.hpp"
#include "resource.h"

HRESULT MainPickerContext::DrawTwoDimensionalGradient(const D2D1_COLOR_F &top_left, const D2D1_COLOR_F &top_right, const D2D1_COLOR_F &bottom_left, const D2D1_COLOR_F &bottom_right)
{
	const D2D1_GRADIENT_MESH_PATCH meshPatch = D2D1::GradientMeshPatch(
		D2D1::Point2F(),
		D2D1::Point2F(m_size.width / 2.0f),
		D2D1::Point2F(m_size.width / 2.0f),
		D2D1::Point2F(m_size.width),
		D2D1::Point2F(0.0f, m_size.height / 2.0f),
		D2D1::Point2F(m_size.width / 3.0f, m_size.height / 3.0f),
		D2D1::Point2F(m_size.width / 3.0f * 2.0f, m_size.height / 3.0f),
		D2D1::Point2F(m_size.width, m_size.height / 2.0f),
		D2D1::Point2F(0.0f, m_size.height / 2.0f),
		D2D1::Point2F(m_size.width / 3.0f, m_size.height / 3.0f * 2.0f),
		D2D1::Point2F(m_size.width / 3.0f * 2.0f, m_size.height / 3.0f * 2.0f),
		D2D1::Point2F(m_size.width, m_size.height / 2.0f),
		D2D1::Point2F(0.0f, m_size.height),
		D2D1::Point2F(m_size.width / 2.0f, m_size.height),
		D2D1::Point2F(m_size.width / 2.0f, m_size.height),
		D2D1::Point2F(m_size.width, m_size.height),
		top_left,
		top_right,
		bottom_left,
		bottom_right,
		D2D1_PATCH_EDGE_MODE_ANTIALIASED,
		D2D1_PATCH_EDGE_MODE_ANTIALIASED,
		D2D1_PATCH_EDGE_MODE_ANTIALIASED,
		D2D1_PATCH_EDGE_MODE_ANTIALIASED
	);

	winrt::com_ptr<ID2D1GradientMesh> mesh;
	const HRESULT hr = m_dc->CreateGradientMesh(&meshPatch, 1, mesh.put());
	if (FAILED(hr))
	{
		return hr;
	}
	m_dc->DrawGradientMesh(mesh.get());

	return S_OK;
}

void MainPickerContext::Destroy()
{
	m_transparentToBlack = nullptr;
	m_transparentToWhite = nullptr;
	m_hueGradient = nullptr;

	RenderContext::Destroy();
}

HRESULT MainPickerContext::Refresh(HWND hwnd)
{
	HRESULT hr;

	hr = RenderContext::Refresh(hwnd);
	if (FAILED(hr))
	{
		return hr;
	}

	hr = CreateGradient(m_transparentToBlack.put(), D2D1::ColorF(0, 0.0f), D2D1::ColorF(D2D1::ColorF::Black));
	if (FAILED(hr))
	{
		return hr;
	}

	hr = CreateGradient(m_transparentToWhite.put(), D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.0f), D2D1::ColorF(D2D1::ColorF::White));
	if (FAILED(hr))
	{
		return hr;
	}

	hr = CreateHueGradient(m_dc.get(), m_hueGradient.put());
	if (FAILED(hr))
	{
		return hr;
	}

	return S_OK;
}

HRESULT MainPickerContext::Draw(HWND hDlg, const SColourF &col, const SColourF &)
{
	DrawContext dc = BeginDraw();

	HRESULT hr;

	// RED
	if (IsDlgButtonChecked(hDlg, IDC_R) == BST_CHECKED)
	{
		hr = DrawTwoDimensionalGradient(D2D1::ColorF(col.r, 0.0f, 0.0f), D2D1::ColorF(col.r, 1.0f, 0.0f), D2D1::ColorF(col.r, 0.0f, 1.0f), D2D1::ColorF(col.r, 1.0f, 1.0f));
		if (FAILED(hr))
		{
			return hr;
		}
	}

	// GREEN
	else if (IsDlgButtonChecked(hDlg, IDC_G) == BST_CHECKED)
	{
		hr = DrawTwoDimensionalGradient(D2D1::ColorF(0.0f, col.g, 0.0f), D2D1::ColorF(1.0f, col.g, 0.0f), D2D1::ColorF(0.0f, col.g, 1.0f), D2D1::ColorF(1.0f, col.g, 1.0f));
		if (FAILED(hr))
		{
			return hr;
		}
	}

	// BLUE
	else if (IsDlgButtonChecked(hDlg, IDC_B) == BST_CHECKED)
	{
		hr = DrawTwoDimensionalGradient(D2D1::ColorF(0.0f, 0.0f, col.b), D2D1::ColorF(0.0f, 1.0f, col.b), D2D1::ColorF(1.0f, 0.0f, col.b), D2D1::ColorF(1.0f, 1.0f, col.b));
		if (FAILED(hr))
		{
			return hr;
		}
	}

	// HUE
	else if (IsDlgButtonChecked(hDlg, IDC_H) == BST_CHECKED)
	{
		SColour temp;
		temp.h = col.h * 359.0f;
		temp.s = 100;
		temp.v = 100;
		temp.UpdateRGB();
		hr = DrawTwoDimensionalGradient(D2D1::ColorF(D2D1::ColorF::White), D2D1::ColorF(temp.r / 255.0f, temp.g / 255.0f, temp.b / 255.0f), D2D1::ColorF(D2D1::ColorF::Black), D2D1::ColorF(D2D1::ColorF::Black));
		if (FAILED(hr))
		{
			return hr;
		}
	}

	// SATURATION
	else if (IsDlgButtonChecked(hDlg, IDC_S) == BST_CHECKED)
	{
		m_dc->FillRectangle(D2D1::RectF(0.0f, 0.0f, m_size.width, m_size.height), m_hueGradient.get());

		m_brush->SetColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f - col.s));
		m_dc->FillRectangle(D2D1::RectF(0.0f, 0.0f, m_size.width, m_size.height), m_brush.get());

		m_dc->FillRectangle(D2D1::RectF(0.0f, 0.0f, m_size.width, m_size.height), m_transparentToBlack.get());
	}

	// VALUE
	else if (IsDlgButtonChecked(hDlg, IDC_V) == BST_CHECKED)
	{
		m_dc->FillRectangle(D2D1::RectF(0.0f, 0.0f, m_size.width, m_size.height), m_hueGradient.get());
		m_dc->FillRectangle(D2D1::RectF(0.0f, 0.0f, m_size.width, m_size.height), m_transparentToWhite.get());

		m_brush->SetColor(D2D1::ColorF(0, 1.0f - col.v));
		m_dc->FillRectangle(D2D1::RectF(0.0f, 0.0f, m_size.width, m_size.height), m_brush.get());
	}

	return dc.EndDraw();
}