#include "main.hpp"
#include <algorithm>
#include <d2d1.h>
#include <iomanip>
#include <sstream>
#include <string>
#include <unordered_map>
#include <WinUser.h>
#include <CommCtrl.h>

#include "CColourPicker.hpp"
#include "drawhelper.hpp"
#include "drawroutines.hpp"
#include "PickerData.hpp"
#include "resource.h"
#include "SColour.hpp"
#include "../TranslucentTB/util.hpp"

static const std::unordered_map<unsigned int, const std::pair<const unsigned int, const unsigned int>> SLIDER_MAP = {
	{ IDC_RED,        { IDC_RSLIDER,   255 } },
	{ IDC_GREEN,      { IDC_GSLIDER,   255 } },
	{ IDC_BLUE,       { IDC_BSLIDER,   255 } },
	{ IDC_ALPHA,      { IDC_ASLIDER,   255 } },
	{ IDC_HUE,        { IDC_HSLIDER,   359 } },
	{ IDC_SATURATION, { IDC_SSLIDER,   100 } },
	{ IDC_VALUE,      { IDC_VSLIDER,   100 } },
	{ IDC_HEXCOL,     { IDC_HEXSLIDER, 0xFFFFFFFF } }
};

int CALLBACK ColourPickerDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PickerData *picker_data = reinterpret_cast<PickerData *>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
	if (!picker_data && uMsg != WM_INITDIALOG)
	{
		return 0;
	}

	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
		picker_data = reinterpret_cast<PickerData *>(lParam);

		D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &picker_data->factory);

		CColourPicker::PickerMap[&(picker_data->picker->Value)] = hDlg;

		for (const decltype(SLIDER_MAP)::value_type &slider_combo : SLIDER_MAP)
		{
			SendDlgItemMessage(hDlg, slider_combo.second.first, UDM_SETBUDDY, (WPARAM)GetDlgItem(hDlg, slider_combo.first), 0);
			SendDlgItemMessage(hDlg, slider_combo.second.first, UDM_SETRANGE32, 0, slider_combo.second.second);
		}
		SendDlgItemMessage(hDlg, IDC_HEXSLIDER, UDM_SETBASE, 16, 0);
		SendDlgItemMessage(hDlg, IDC_HEXCOL, EM_SETLIMITTEXT, 10, 0);

		UpdateValues(hDlg, picker_data->picker->GetCurrentColour(), picker_data->changing_text);

		SendDlgItemMessage(hDlg, IDC_R, BM_SETCHECK, BST_CHECKED, 0);

		[[fallthrough]];
	}

	case WM_DPICHANGED:
	{
		RECT rect;
		HWND item;


		picker_data->targetC1.Release();
		item = GetDlgItem(hDlg, IDC_COLOR);
		GetClientRect(item, &rect);
		picker_data->factory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(item, D2D1::SizeU(rect.right, rect.bottom)), &picker_data->targetC1);

		picker_data->targetC2.Release();
		item = GetDlgItem(hDlg, IDC_COLOR2);
		GetClientRect(item, &rect);
		picker_data->factory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(item, D2D1::SizeU(rect.right, rect.bottom)), &picker_data->targetC2);

		picker_data->targetA.Release();
		item = GetDlgItem(hDlg, IDC_ALPHASLIDE);
		GetClientRect(item, &rect);
		picker_data->factory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(item, D2D1::SizeU(rect.right, rect.bottom)), &picker_data->targetA);

		picker_data->targetCC.Release();
		item = GetDlgItem(hDlg, IDC_CURRCOLOR);
		GetClientRect(item, &rect);
		picker_data->factory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(item, D2D1::SizeU(rect.right, rect.bottom)), &picker_data->targetCC);

		picker_data->targetOC.Release();
		item = GetDlgItem(hDlg, IDC_OLDCOLOR);
		GetClientRect(item, &rect);
		picker_data->factory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(item, D2D1::SizeU(rect.right, rect.bottom)), &picker_data->targetOC);


		RedrawWindow(hDlg, NULL, NULL, RDW_UPDATENOW | RDW_INTERNALPAINT);

		break;
	}
	case WM_PAINT:
	{
		// Check if we need to recreate the render targets.
		picker_data->targetC1->BeginDraw();
		if (picker_data->targetC1->EndDraw() == D2DERR_RECREATE_TARGET)
		{
			// WM_DPICHANGED refreshes our targets and does a redraw.
			return SendMessage(hDlg, WM_DPICHANGED, NULL, NULL);
		}

		const SColour &color = picker_data->picker->GetCurrentColour();
		const float rf = color.r / 255.0f;
		const float gf = color.g / 255.0f;
		const float bf = color.b / 255.0f;
		const float af = color.a / 255.0f;

		PAINTSTRUCT ps;
		BeginPaint(hDlg, &ps);

		DrawColorPicker(picker_data->targetC1, hDlg, rf, gf, bf, color.h, color.s, color.v);

		// Small color selector (displays selected feature)
		DrawColorSlider(picker_data->targetC2, hDlg, rf, gf, bf, color.h, color.s, color.v);

		// Alpha slider
		DrawAlphaSlider(picker_data->targetA, D2D1::ColorF(rf, gf, bf), 1.0f - af);

		DrawColorIndicator(picker_data->targetCC, color);
		DrawColorIndicator(picker_data->targetOC, picker_data->picker->GetOldColour());

		EndPaint(hDlg, &ps);

		break;
	}

	case WM_LBUTTONDOWN:
	case WM_MOUSEMOVE:
	{
		if (uMsg == WM_MOUSEMOVE && wParam != MK_LBUTTON)
		{
			break;
		}

		RECT rectC1;
		RECT rectC2;
		RECT rectA;

		GetWindowRect(GetDlgItem(hDlg, IDC_COLOR), &rectC1);
		GetWindowRect(GetDlgItem(hDlg, IDC_COLOR2), &rectC2);
		GetWindowRect(GetDlgItem(hDlg, IDC_ALPHASLIDE), &rectA);

		BOOL result;
		const uint8_t red   = SendDlgItemMessage(hDlg, IDC_RSLIDER, UDM_GETPOS, 0, (LPARAM)&result);
		const uint8_t green = SendDlgItemMessage(hDlg, IDC_GSLIDER, UDM_GETPOS, 0, (LPARAM)&result);
		const uint8_t blue  = SendDlgItemMessage(hDlg, IDC_BSLIDER, UDM_GETPOS, 0, (LPARAM)&result);

		const unsigned short hue = SendDlgItemMessage(hDlg, IDC_HSLIDER, UDM_GETPOS, 0, (LPARAM)&result);
		const uint8_t saturation = SendDlgItemMessage(hDlg, IDC_SSLIDER, UDM_GETPOS, 0, (LPARAM)&result);
		const uint8_t value      = SendDlgItemMessage(hDlg, IDC_VSLIDER, UDM_GETPOS, 0, (LPARAM)&result);

		POINT p;
		GetCursorPos(&p);

		// IDC_COLOR1 picked
		if (PtInRect(&rectC1, p))
		{
			const float fx = ((p.x - rectC1.left) / (float)(rectC1.right - rectC1.left)) * 255.0f;
			const float fy = ((p.y - rectC1.top) / (float)(rectC1.bottom - rectC1.top)) * 255.0f;

			if (IsDlgButtonChecked(hDlg, IDC_R) == BST_CHECKED)
			{
				picker_data->picker->SetRGB(red, fx, fy);
			}

			else if (IsDlgButtonChecked(hDlg, IDC_G) == BST_CHECKED)
			{
				picker_data->picker->SetRGB(fx, green, fy);
			}

			else if (IsDlgButtonChecked(hDlg, IDC_B) == BST_CHECKED)
			{
				picker_data->picker->SetRGB(fy, fx, blue);
			}

			else if (IsDlgButtonChecked(hDlg, IDC_H) == BST_CHECKED)
			{
				picker_data->picker->SetHSV(hue, fx / 255.0f * 100.0f, (255 - fy) / 255.0f * 100.0f);
			}

			else if (IsDlgButtonChecked(hDlg, IDC_S) == BST_CHECKED)
			{
				picker_data->picker->SetHSV(fx / 255.0f * 359.0f, saturation, (255 - fy) / 255.0f * 100.0f);
			}

			else if (IsDlgButtonChecked(hDlg, IDC_V) == BST_CHECKED)
			{
				picker_data->picker->SetHSV(fx / 255.0f * 359.0f, (255 - fy) / 255.0f * 100.0f, value);
			}
		}
		// IDC_COLOR2 picked
		else if (PtInRect(&rectC2, p))
		{
			const float fy = ((p.y - rectC2.top) / (float)(rectC2.bottom - rectC2.top)) * 255.0f;

			if (IsDlgButtonChecked(hDlg, IDC_R) == BST_CHECKED)
			{
				picker_data->picker->SetRGB(255 - fy, green, blue);
			}

			else if (IsDlgButtonChecked(hDlg, IDC_G) == BST_CHECKED)
			{
				picker_data->picker->SetRGB(red, 255 - fy, blue);
			}

			else if (IsDlgButtonChecked(hDlg, IDC_B) == BST_CHECKED)
			{
				picker_data->picker->SetRGB(red, green, 255 - fy);
			}

			else if (IsDlgButtonChecked(hDlg, IDC_H) == BST_CHECKED)
			{
				picker_data->picker->SetHSV((255 - fy) / 255.0f * 359.0f, saturation, value);
			}

			else if (IsDlgButtonChecked(hDlg, IDC_S) == BST_CHECKED)
			{
				picker_data->picker->SetHSV(hue, (255 - fy) / 255.0f * 100.0f, value);
			}

			else if (IsDlgButtonChecked(hDlg, IDC_V) == BST_CHECKED)
			{
				picker_data->picker->SetHSV(hue, saturation, (255 - fy) / 255.0f * 100.0f);
			}
		}
		// IDC_ALPHASLIDE picked
		else if (PtInRect(&rectA, p))
		{
			const float fy = ((p.y - rectA.top) / (float)(rectA.bottom - rectA.top)) * 255.0f;

			picker_data->picker->SetAlpha(255 - fy);
		}

		UpdateValues(hDlg, picker_data->picker->GetCurrentColour(), picker_data->changing_text);
		RedrawWindow(hDlg, NULL, NULL, RDW_UPDATENOW | RDW_INTERNALPAINT);
		break;
	}

	case WM_COMMAND:
		switch (HIWORD(wParam))
		{
		case EN_SETFOCUS:
		{
			if (LOWORD(wParam) != IDC_HEXCOL)
			{
				if (GetDlgItemInt(hDlg, LOWORD(wParam), NULL, FALSE) == 0)
				{
					SetDlgItemText(hDlg, LOWORD(wParam), L"");
				}
			}
			SendDlgItemMessage(hDlg, LOWORD(wParam), EM_SETSEL, 0, -1);
			break;
		}

		case EN_KILLFOCUS:
		{
			if (LOWORD(wParam) == IDC_HEXCOL)
			{
				wchar_t rawText[11];
				GetDlgItemText(hDlg, IDC_HEXCOL, rawText, 11);

				std::wstring text = Util::Trim(rawText);
				if (text[0] == L'#')
				{
					text.erase(0, 1);
				}
				else if (text[0] == L'0' && text[1] == L'x')
				{
					text.erase(0, 2);
				}

				try
				{
					const unsigned int tempcolor = std::stoll(text, nullptr, 16) & 0xFFFFFFFF;

					if (text.length() == 8)
					{
						picker_data->picker->SetRGB((tempcolor & 0xFF000000) >> 24, (tempcolor & 0x00FF0000) >> 16, (tempcolor & 0x0000FF00) >> 8);
						picker_data->picker->SetAlpha(tempcolor & 0x000000FF);
					}
					else if (text.length() == 4)
					{
						picker_data->picker->SetRGB(ExpandOneLetterByte((tempcolor & 0xF000) >> 12), ExpandOneLetterByte((tempcolor & 0x0F00) >> 8), ExpandOneLetterByte((tempcolor & 0x00F0) >> 4));
						picker_data->picker->SetAlpha(ExpandOneLetterByte(tempcolor & 0x000F));
					}
					else if (text.length() == 6)
					{
						picker_data->picker->SetRGB((tempcolor & 0xFF0000) >> 16, (tempcolor & 0x00FF00) >> 8, tempcolor & 0x0000FF);
					}
					else if (text.length() == 3)
					{
						picker_data->picker->SetRGB(ExpandOneLetterByte((tempcolor & 0xF00) >> 8), ExpandOneLetterByte((tempcolor & 0x0F0) >> 4), ExpandOneLetterByte(tempcolor & 0x00F));
					}

					UpdateValues(hDlg, picker_data->picker->GetCurrentColour(), picker_data->changing_text);
					RedrawWindow(hDlg, NULL, NULL, RDW_UPDATENOW | RDW_INTERNALPAINT);
				}
				catch (std::invalid_argument) { } // Fight me
			}

			picker_data->changing_text = true;
			for (const decltype(SLIDER_MAP)::value_type &slider_combo : SLIDER_MAP)
			{
				if (slider_combo.first == IDC_HEXCOL)
				{
					const SColour &col = picker_data->picker->GetCurrentColour();

					std::wostringstream stream;
					stream << L"0x";
					stream << std::setw(2) << std::setfill(L'0') << std::hex << col.r << col.g << col.b << col.a;

					SetDlgItemText(hDlg, slider_combo.first, stream.str().c_str());
				}
				else
				{
					BOOL result;
					SetDlgItemInt(hDlg, slider_combo.first, SendDlgItemMessage(hDlg, slider_combo.second.first, UDM_GETPOS, 0, (LPARAM)&result), false);
				}
			}
			picker_data->changing_text = false;
			break;
		}

		case EN_CHANGE:
		{
			if (picker_data->changing_text)
			{
				break;
			}

			switch (LOWORD(wParam))
			{
				BOOL result;
			case IDC_RED:
			case IDC_GREEN:
			case IDC_BLUE:
			{
				picker_data->picker->SetRGB(SendDlgItemMessage(hDlg, IDC_RSLIDER, UDM_GETPOS, 0, (LPARAM)&result), SendDlgItemMessage(hDlg, IDC_GSLIDER, UDM_GETPOS, 0, (LPARAM)&result), SendDlgItemMessage(hDlg, IDC_BSLIDER, UDM_GETPOS, 0, (LPARAM)&result));
				break;
			}

			case IDC_HUE:
			case IDC_SATURATION:
			case IDC_VALUE:
			{
				picker_data->picker->SetHSV(SendDlgItemMessage(hDlg, IDC_HSLIDER, UDM_GETPOS, 0, (LPARAM)&result), SendDlgItemMessage(hDlg, IDC_SSLIDER, UDM_GETPOS, 0, (LPARAM)&result), SendDlgItemMessage(hDlg, IDC_VSLIDER, UDM_GETPOS, 0, (LPARAM)&result));
				break;
			}

			case IDC_ALPHA:
			{
				picker_data->picker->SetAlpha(SendDlgItemMessage(hDlg, IDC_ASLIDER, UDM_GETPOS, 0, (LPARAM)&result));
				break;
			}

			case IDC_HEXCOL:
			{
				return 0;
			}
			}

			// Update color
			UpdateValues(hDlg, picker_data->picker->GetCurrentColour(), picker_data->changing_text);
			RedrawWindow(hDlg, NULL, NULL, RDW_UPDATENOW | RDW_INTERNALPAINT);

			break;
		}

		case BN_CLICKED: // Equivalent to STN_CLICKED
		{
			switch (LOWORD(wParam))
			{
			case IDC_R:
			case IDC_B:
			case IDC_G:
			case IDC_H:
			case IDC_S:
			case IDC_V:
			{
				RedrawWindow(hDlg, NULL, NULL, RDW_UPDATENOW | RDW_INTERNALPAINT);
				break;
			}

			case IDC_OLDCOLOR:
			{
				const SColour &old = picker_data->picker->GetOldColour();

				picker_data->picker->SetRGB(old.r, old.g, old.b);
				picker_data->picker->SetAlpha(old.a);
				UpdateValues(hDlg, picker_data->picker->GetCurrentColour(), picker_data->changing_text);
				RedrawWindow(hDlg, NULL, NULL, RDW_UPDATENOW | RDW_INTERNALPAINT);
				break;
			}

			case IDB_OK:
			{
				picker_data->picker->UpdateOldColour();
				EndDialog(hDlg, IDB_OK);
				break;
			}

			case IDB_CANCEL:
			{
				const SColour &old = picker_data->picker->GetOldColour();

				picker_data->picker->SetRGB(old.r, old.g, old.b);
				picker_data->picker->SetAlpha(old.a);
				EndDialog(hDlg, IDB_CANCEL);
				break;
			}
			}

			break;
		}
		}
		break;
	}
	return 0;
}

uint8_t ExpandOneLetterByte(const uint8_t &byte)
{
	const uint8_t firstDigit = byte & 0xF;
	return (firstDigit << 4) + firstDigit;
}

void UpdateValues(HWND hDlg, const SColour &col, bool &changing_text)
{
	changing_text = true;

	SendDlgItemMessage(hDlg, IDC_RSLIDER, UDM_SETPOS, 0, col.r);
	SendDlgItemMessage(hDlg, IDC_GSLIDER, UDM_SETPOS, 0, col.g);
	SendDlgItemMessage(hDlg, IDC_BSLIDER, UDM_SETPOS, 0, col.b);
	SendDlgItemMessage(hDlg, IDC_ASLIDER, UDM_SETPOS, 0, col.a);
	SendDlgItemMessage(hDlg, IDC_HSLIDER, UDM_SETPOS, 0, col.h);
	SendDlgItemMessage(hDlg, IDC_SSLIDER, UDM_SETPOS, 0, col.s);
	SendDlgItemMessage(hDlg, IDC_VSLIDER, UDM_SETPOS, 0, col.v);
	SendDlgItemMessage(hDlg, IDC_HEXSLIDER, UDM_SETPOS32, 0, (col.r << 24) + (col.g << 16) + (col.b << 8) + col.a);

	changing_text = false;
}