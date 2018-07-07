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

	CComPtr<ID2D1GradientMesh> mesh;
	const HRESULT hr = m_dc->CreateGradientMesh(&meshPatch, 1, &mesh);
	if (FAILED(hr))
	{
		return hr;
	}
	m_dc->DrawGradientMesh(mesh);

	return S_OK;
}

void MainPickerContext::ReleaseAll()
{
	m_hueGradient.Release();
	m_transparentToBlack.Release();
	m_transparentToWhite.Release();

	RenderContext::ReleaseAll();
}

HRESULT MainPickerContext::Refresh(HWND hwnd)
{
	HRESULT hr;

	hr = RenderContext::Refresh(hwnd);
	if (FAILED(hr))
	{
		return hr;
	}

	CreateGradient(m_transparentToBlack, D2D1::ColorF(0, 0.0f), D2D1::ColorF(D2D1::ColorF::Black));
	CreateGradient(m_transparentToWhite, D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.0f), D2D1::ColorF(D2D1::ColorF::White));

	hr = CreateHueGradient(m_dc, m_hueGradient);
	if (FAILED(hr))
	{
		return hr;
	}

	// TODO: bw gradients

	return S_OK;
}

HRESULT MainPickerContext::Draw(const HWND hDlg, const SColourF &col, const SColourF &)
{
	m_dc->BeginDraw();

	D2D1_POINT_2F indicator_point;

	// RED
	if (IsDlgButtonChecked(hDlg, IDC_R) == BST_CHECKED)
	{
		DrawTwoDimensionalGradient(D2D1::ColorF(col.r, 0.0f, 0.0f), D2D1::ColorF(col.r, 1.0f, 0.0f), D2D1::ColorF(col.r, 0.0f, 1.0f), D2D1::ColorF(col.r, 1.0f, 1.0f));

		indicator_point = D2D1::Point2F(col.g * m_size.width, col.b * m_size.height);
	}

	// GREEN
	else if (IsDlgButtonChecked(hDlg, IDC_G) == BST_CHECKED)
	{
		DrawTwoDimensionalGradient(D2D1::ColorF(0.0f, col.g, 0.0f), D2D1::ColorF(1.0f, col.g, 0.0f), D2D1::ColorF(0.0f, col.g, 1.0f), D2D1::ColorF(1.0f, col.g, 1.0f));

		indicator_point = D2D1::Point2F(col.r * m_size.width, col.b * m_size.height);
	}

	// BLUE
	else if (IsDlgButtonChecked(hDlg, IDC_B) == BST_CHECKED)
	{
		DrawTwoDimensionalGradient(D2D1::ColorF(0.0f, 0.0f, col.b), D2D1::ColorF(0.0f, 1.0f, col.b), D2D1::ColorF(1.0f, 0.0f, col.b), D2D1::ColorF(1.0f, 1.0f, col.b));

		indicator_point = D2D1::Point2F(col.g * m_size.width, col.r * m_size.height);
	}

	// HUE
	else if (IsDlgButtonChecked(hDlg, IDC_H) == BST_CHECKED)
	{
		SColour temp;
		temp.h = col.h * 100.0f;
		temp.s = 100;
		temp.v = 100;
		temp.UpdateRGB();
		DrawTwoDimensionalGradient(D2D1::ColorF(D2D1::ColorF::White), D2D1::ColorF(temp.r / 255.0f, temp.g / 255.0f, temp.b / 255.0f), D2D1::ColorF(D2D1::ColorF::Black), D2D1::ColorF(D2D1::ColorF::Black));

		indicator_point = D2D1::Point2F(col.s * m_size.width, (1.0f - col.v) * m_size.height);
	}

	// SATURATION
	else if (IsDlgButtonChecked(hDlg, IDC_S) == BST_CHECKED)
	{
		m_dc->FillRectangle(D2D1::RectF(0.0f, 0.0f, m_size.width, m_size.height), m_hueGradient);

		m_brush->SetColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f - col.s));
		m_dc->FillRectangle(D2D1::RectF(0.0f, 0.0f, m_size.width, m_size.height), m_brush);

		m_dc->FillRectangle(D2D1::RectF(0.0f, 0.0f, m_size.width, m_size.height), m_transparentToBlack);

		indicator_point = D2D1::Point2F(col.h * m_size.width, (1.0f - col.v) * m_size.height);
	}

	// VALUE
	else if (IsDlgButtonChecked(hDlg, IDC_V) == BST_CHECKED)
	{
		m_dc->FillRectangle(D2D1::RectF(0.0f, 0.0f, m_size.width, m_size.height), m_hueGradient);
		m_dc->FillRectangle(D2D1::RectF(0.0f, 0.0f, m_size.width, m_size.height), m_transparentToWhite);

		m_brush->SetColor(D2D1::ColorF(0, 1.0f - col.v));
		m_dc->FillRectangle(D2D1::RectF(0.0f, 0.0f, m_size.width, m_size.height), m_brush);

		indicator_point = D2D1::Point2F(col.h * m_size.width, (1.0f - col.s) * m_size.height);
	}

	m_brush->SetColor(D2D1::ColorF(1.0f - col.r, 1.0f - col.g, 1.0f - col.b));
	const float circle_radius = m_size.width / 50.0f;
	m_dc->DrawEllipse(D2D1::Ellipse(indicator_point, circle_radius, circle_radius), m_brush);

	return EndDraw();
}