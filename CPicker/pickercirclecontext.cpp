#include "pickercirclecontext.hpp"
#include "resource.h"

HRESULT PickerCircleContext::Draw(HWND hDlg, const SColourF &col)
{
	DrawContext dc = BeginDraw();
	m_dc->Clear(D2D1::ColorF(0, 0.0f));

	D2D1_POINT_2F indicator_point;

	// RED
	if (IsDlgButtonChecked(hDlg, IDC_R) == BST_CHECKED)
	{
		indicator_point = D2D1::Point2F(col.g, col.b);
	}

	// GREEN
	else if (IsDlgButtonChecked(hDlg, IDC_G) == BST_CHECKED)
	{
		indicator_point = D2D1::Point2F(col.r, col.b);
	}

	// BLUE
	else if (IsDlgButtonChecked(hDlg, IDC_B) == BST_CHECKED)
	{
		indicator_point = D2D1::Point2F(col.g, col.r);
	}

	// HUE
	else if (IsDlgButtonChecked(hDlg, IDC_H) == BST_CHECKED)
	{
		indicator_point = D2D1::Point2F(col.s, (1.0f - col.v));
	}

	// SATURATION
	else if (IsDlgButtonChecked(hDlg, IDC_S) == BST_CHECKED)
	{
		indicator_point = D2D1::Point2F(col.h, (1.0f - col.v));
	}

	// VALUE
	else if (IsDlgButtonChecked(hDlg, IDC_V) == BST_CHECKED)
	{
		indicator_point = D2D1::Point2F(col.h, (1.0f - col.s));
	}

	const float offset_x = m_size.width / 52.0f;
	const float width = m_size.width - (offset_x * 2.0f);
	const float offset_y = m_size.height / 52.0f;
	const float height = m_size.height - (offset_y * 2.0f);

	indicator_point.x *= width;
	indicator_point.x += offset_x;

	indicator_point.y *= height;
	indicator_point.y += offset_y;

	m_brush->SetColor(D2D1::ColorF(1.0f - col.r, 1.0f - col.g, 1.0f - col.b));
	m_dc->DrawEllipse(D2D1::Ellipse(indicator_point, offset_x, offset_x), m_brush.get());

	return dc.EndDraw();
}