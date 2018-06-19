#include "main.hpp"
#include <algorithm>
#include <stdio.h>
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

static const Util::string_map<uint32_t> COLOR_MAP = {
	{ L"aliceblue",				0xf0f8ff },
	{ L"antiquewhite",			0xfaebd7 },
	{ L"aqua",					0x00ffff },
	{ L"aquamarine",			0x7fffd4 },
	{ L"azure",					0xf0ffff },
	{ L"beige",					0xf5f5dc },
	{ L"bisque",				0xffe4c4 },
	{ L"black",					0x000000 },
	{ L"blanchedalmond",		0xffebcd },
	{ L"blue",					0x0000ff },
	{ L"blueviolet",			0x8a2be2 },
	{ L"brown",					0xa52a2a },
	{ L"burlywood",				0xdeb887 },
	{ L"cadetblue",				0x5f9ea0 },
	{ L"chartreuse",			0x7fff00 },
	{ L"chocolate",				0xd2691e },
	{ L"coral",					0xff7f50 },
	{ L"cornflowerblue",		0x6495ed },
	{ L"cornsilk",				0xfff8dc },
	{ L"crimson",				0xdc143c },
	{ L"cyan",					0x00ffff },
	{ L"darkblue",				0x00008b },
	{ L"darkcyan",				0x008b8b },
	{ L"darkgoldenrod",			0xb8860b },
	{ L"darkgray",				0xa9a9a9 },
	{ L"darkgreen",				0x006400 },
	{ L"darkgrey",				0xa9a9a9 },
	{ L"darkkhaki",				0xbdb76b },
	{ L"darkmagenta",			0x8b008b },
	{ L"darkolivegreen",		0x556b2f },
	{ L"darkorange",			0xff8c00 },
	{ L"darkorchid",			0x9932cc },
	{ L"darkred",				0x8b0000 },
	{ L"darksalmon",			0xe9967a },
	{ L"darkseagreen",			0x8fbc8f },
	{ L"darkslateblue",			0x483d8b },
	{ L"darkslategray",			0x2f4f4f },
	{ L"darkslategrey",			0x2f4f4f },
	{ L"darkturquoise",			0x00ced1 },
	{ L"darkviolet",			0x9400d3 },
	{ L"deeppink",				0xff1493 },
	{ L"deepskyblue",			0x00bfff },
	{ L"dimgray",				0x696969 },
	{ L"dimgrey",				0x696969 },
	{ L"dodgerblue",			0x1e90ff },
	{ L"firebrick",				0xb22222 },
	{ L"floralwhite",			0xfffaf0 },
	{ L"forestgreen",			0x228b22 },
	{ L"fuchsia",				0xff00ff },
	{ L"gainsboro",				0xdcdcdc },
	{ L"ghostwhite",			0xf8f8ff },
	{ L"gold",					0xffd700 },
	{ L"goldenrod",				0xdaa520 },
	{ L"gray",					0x808080 },
	{ L"green",					0x008000 },
	{ L"greenyellow",			0xadff2f },
	{ L"grey",					0x808080 },
	{ L"honeydew",				0xf0fff0 },
	{ L"hotpink",				0xff69b4 },
	{ L"indianred",				0xcd5c5c },
	{ L"indigo",				0x4b0082 },
	{ L"ivory",					0xfffff0 },
	{ L"khaki",					0xf0e68c },
	{ L"lavender",				0xe6e6fa },
	{ L"lavenderblush",			0xfff0f5 },
	{ L"lawngreen",				0x7cfc00 },
	{ L"lemonchiffon",			0xfffacd },
	{ L"lightblue",				0xadd8e6 },
	{ L"lightcoral",			0xf08080 },
	{ L"lightcyan",				0xe0ffff },
	{ L"lightgoldenrodyellow",	0xfafad2 },
	{ L"lightgray",				0xd3d3d3 },
	{ L"lightgreen",			0x90ee90 },
	{ L"lightgrey",				0xd3d3d3 },
	{ L"lightpink",				0xffb6c1 },
	{ L"lightsalmon",			0xffa07a },
	{ L"lightseagreen",			0x20b2aa },
	{ L"lightskyblue",			0x87cefa },
	{ L"lightslategray",		0x778899 },
	{ L"lightslategrey",		0x778899 },
	{ L"lightsteelblue",		0xb0c4de },
	{ L"lightyellow",			0xffffe0 },
	{ L"lime",					0x00ff00 },
	{ L"limegreen",				0x32cd32 },
	{ L"linen",					0xfaf0e6 },
	{ L"magenta",				0xff00ff },
	{ L"maroon",				0x800000 },
	{ L"mediumaquamarine",		0x66cdaa },
	{ L"mediumblue",			0x0000cd },
	{ L"mediumorchid",			0xba55d3 },
	{ L"mediumpurple",			0x9370db },
	{ L"mediumseagreen",		0x3cb371 },
	{ L"mediumslateblue",		0x7b68ee },
	{ L"mediumspringgreen",		0x00fa9a },
	{ L"mediumturquoise",		0x48d1cc },
	{ L"mediumvioletred",		0xc71585 },
	{ L"midnightblue",			0x191970 },
	{ L"mintcream",				0xf5fffa },
	{ L"mistyrose",				0xffe4e1 },
	{ L"moccasin",				0xffe4b5 },
	{ L"navajowhite",			0xffdead }, // This color is just like me inside while making this map
	{ L"navy",					0x000080 },
	{ L"oldlace",				0xfdf5e6 },
	{ L"olive",					0x808000 },
	{ L"olivedrab",				0x6b8e23 },
	{ L"orange",				0xffa500 },
	{ L"orangered",				0xff4500 },
	{ L"orchid",				0xda70d6 },
	{ L"palegoldenrod",			0xeee8aa },
	{ L"palegreen",				0x98fb98 },
	{ L"paleturquoise",			0xafeeee },
	{ L"palevioletred",			0xdb7093 },
	{ L"papayawhip",			0xffefd5 },
	{ L"peachpuff",				0xffdab9 },
	{ L"peru",					0xcd853f },
	{ L"pink",					0xffc0cb },
	{ L"plum",					0xdda0dd },
	{ L"powderblue",			0xb0e0e6 },
	{ L"purple",				0x800080 },
	{ L"rebeccapurple",			0x663399 },
	{ L"red",					0xff0000 },
	{ L"rosybrown",				0xbc8f8f },
	{ L"royalblue",				0x4169e1 },
	{ L"saddlebrown",			0x8b4513 },
	{ L"salmon",				0xfa8072 },
	{ L"sandybrown",			0xf4a460 },
	{ L"seagreen",				0x2e8b57 },
	{ L"seashell",				0xfff5ee },
	{ L"sienna",				0xa0522d },
	{ L"silver",				0xc0c0c0 },
	{ L"skyblue",				0x87ceeb },
	{ L"slateblue",				0x6a5acd },
	{ L"slategray",				0x708090 },
	{ L"slategrey",				0x708090 },
	{ L"snow",					0xfffafa },
	{ L"springgreen",			0x00ff7f },
	{ L"steelblue",				0x4682b4 },
	{ L"tan",					0xd2b48c },
	{ L"teal",					0x008080 },
	{ L"thistle",				0xd8bfd8 },
	{ L"tomato",				0xff6347 },
	{ L"turquoise",				0x40e0d0 },
	{ L"violet",				0xee82ee },
	{ L"wheat",					0xf5deb3 },
	{ L"white",					0xffffff },
	{ L"whitesmoke",			0xf5f5f5 },
	{ L"yellow",				0xffff00 },
	{ L"yellowgreen",			0x9acd32 }
};

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

INT_PTR CColourPicker::ColourPickerDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PickerData *picker_data = reinterpret_cast<PickerData *>(GetWindowLongPtr(hDlg, DWLP_USER));
	if (!picker_data && uMsg != WM_INITDIALOG)
	{
		return 0;
	}

	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		SetWindowLongPtr(hDlg, DWLP_USER, lParam);
		picker_data = reinterpret_cast<PickerData *>(lParam);

		D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &picker_data->factory);

		CColourPicker::PickerMap[&picker_data->picker->Value] = hDlg;

		for (const auto &slider_combo : SLIDER_MAP)
		{
			SendDlgItemMessage(hDlg, slider_combo.second.first, UDM_SETBUDDY, (WPARAM)GetDlgItem(hDlg, slider_combo.first), 0);
			SendDlgItemMessage(hDlg, slider_combo.second.first, UDM_SETRANGE32, 0, slider_combo.second.second);
		}
		SendDlgItemMessage(hDlg, IDC_HEXSLIDER, UDM_SETBASE, 16, 0);
		SendDlgItemMessage(hDlg, IDC_HEXCOL, EM_SETLIMITTEXT, 20, 0);
		Edit_SetCueBannerTextFocused(GetDlgItem(hDlg, IDC_HEXCOL), L"HTML color", TRUE);

		for (const int &button : { IDC_R, IDC_G, IDC_B, IDC_H, IDC_S, IDC_V })
		{
			picker_data->button_proc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hDlg, button), GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(NoOutlineButtonProc));
		}

		UpdateValues(hDlg, picker_data->picker->GetCurrentColour(), picker_data->changing_text);

		SendDlgItemMessage(hDlg, IDC_R, BM_SETCHECK, BST_CHECKED, 0);

		picker_data->old_color_tip = CreateWindow(TOOLTIPS_CLASS, NULL, TTS_ALWAYSTIP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, hDlg, NULL, Instance, NULL);

		TOOLINFO ti = {
			sizeof(ti),
			TTF_IDISHWND | TTF_SUBCLASS,
			hDlg,
			(UINT_PTR)GetDlgItem(hDlg, IDC_OLDCOLOR),
		};
		ti.lpszText = LPSTR_TEXTCALLBACK;
		SendMessage(picker_data->old_color_tip, TTM_ADDTOOL, 0, (LPARAM)&ti);
		SendMessage(picker_data->old_color_tip, TTM_ACTIVATE, TRUE, 0);

		[[fallthrough]];
	}

	case WM_DPICHANGED:
	{
		CreateTarget(picker_data->factory, picker_data->targetC1, picker_data->brushC1, hDlg, IDC_COLOR);
		CreateTarget(picker_data->factory, picker_data->targetC2, picker_data->brushC2, hDlg, IDC_COLOR2);
		CreateTarget(picker_data->factory, picker_data->targetA, picker_data->brushA, hDlg, IDC_ALPHASLIDE);
		CreateTarget(picker_data->factory, picker_data->targetC, picker_data->brushC, hDlg, IDC_COLORS);

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

		DrawColorPicker(picker_data->targetC1, picker_data->brushC1, hDlg, rf, gf, bf, color.h, color.s, color.v);

		// Small color selector (displays selected feature)
		DrawColorSlider(picker_data->targetC2, picker_data->brushC2, hDlg, rf, gf, bf, color.h, color.s, color.v);

		// Alpha slider
		DrawAlphaSlider(picker_data->targetA, picker_data->brushA, rf, gf, bf, 1.0f - af);

		DrawColorIndicator(picker_data->targetC, picker_data->brushC, rf, gf, bf, af, picker_data->picker->GetOldColour());

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
				ParseHex(hDlg, picker_data->picker);

				UpdateValues(hDlg, picker_data->picker->GetCurrentColour(), picker_data->changing_text);
				RedrawWindow(hDlg, NULL, NULL, RDW_UPDATENOW | RDW_INTERNALPAINT);
			}

			picker_data->changing_text = true;
			for (const auto &slider_combo : SLIDER_MAP)
			{
				if (slider_combo.first == IDC_HEXCOL)
				{
					const SColour &col = picker_data->picker->GetCurrentColour();

					wchar_t buff[11];
					_snwprintf_s(buff, 10, L"0x%02X%02X%02X%02X", col.r, col.g, col.b, col.a);

					SetDlgItemText(hDlg, slider_combo.first, buff);
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
				if (!picker_data->changing_hex_via_spin)
				{
					return 0;
				}
				else
				{
					ParseHex(hDlg, picker_data->picker);
					picker_data->changing_hex_via_spin = false;
				}
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
				DestroyWindow(picker_data->old_color_tip);
				EndDialog(hDlg, IDB_OK);
				break;
			}

			case IDB_CANCEL:
			{
				const SColour &old = picker_data->picker->GetOldColour();

				picker_data->picker->SetRGB(old.r, old.g, old.b);
				picker_data->picker->SetAlpha(old.a);
				DestroyWindow(picker_data->old_color_tip);
				EndDialog(hDlg, IDB_CANCEL);
				break;
			}
			}

			break;
		}
		}
		break;

	case WM_NOTIFY:
	{
		const NMHDR *const notify = reinterpret_cast<const NMHDR *>(lParam);
		switch (notify->code)
		{
		case UDN_DELTAPOS:
		{
			if (notify->idFrom == IDC_HEXSLIDER)
			{
				picker_data->changing_hex_via_spin = true;
			}
			break;
		}

		case TTN_GETDISPINFO:
		{
			NMTTDISPINFO *const dispinfo = reinterpret_cast<NMTTDISPINFO *>(lParam);
			wcscpy_s(dispinfo->szText, L"Click to restore old color");
			break;
		}
		}
		break;
	}
	}
	return 0;
}

LRESULT CALLBACK NoOutlineButtonProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_SETFOCUS)
	{
		return 0;
	}

	const HWND hDlg = GetAncestor(hWnd, GA_PARENT);
	return CallWindowProc(reinterpret_cast<PickerData *>(GetWindowLongPtr(hDlg, DWLP_USER))->button_proc, hWnd, uMsg, wParam, lParam);
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

void FailedParse(HWND hDlg)
{
	EDITBALLOONTIP ebt = {
		sizeof(ebt),
		L"Error when parsing color code!",
		L"Make sure the code is valid hexadecimal. (0x and # prefixes accepted)\n"
		L"Code can be 3 (RGB), 4 (RGBA), 6 (RRGGBB) or 8 (RRGGBBAA) characters.\n\n"
		L"HTML color names are also understood. (for example: yellow, white, blue)",
		TTI_WARNING_LARGE
	};

	Edit_ShowBalloonTip(GetDlgItem(hDlg, IDC_HEXCOL), &ebt);
}

void ParseHex(HWND hDlg, CColourPicker *picker)
{
	std::wstring text;
	text.resize(21);
	int count = GetDlgItemText(hDlg, IDC_HEXCOL, text.data(), 21);
	text.resize(count);
	Util::TrimInplace(text);

	if (COLOR_MAP.count(text) != 0)
	{
		const uint32_t &color = COLOR_MAP.at(text);
		picker->SetRGB((color & 0xFF0000) >> 16, (color & 0x00FF00) >> 8, color & 0x0000FF);
	}
	else
	{
		if (text.length() > 10)
		{
			FailedParse(hDlg);
			return;
		}

		Util::RemovePrefixInplace(text, L"#");
		Util::RemovePrefixInplace(text, L"0x");

		try
		{
			const unsigned int tempcolor = std::stoll(text, nullptr, 16) & 0xFFFFFFFF;

			if (text.length() == 8)
			{
				picker->SetRGB((tempcolor & 0xFF000000) >> 24, (tempcolor & 0x00FF0000) >> 16, (tempcolor & 0x0000FF00) >> 8);
				picker->SetAlpha(tempcolor & 0x000000FF);
			}
			else if (text.length() == 4)
			{
				picker->SetRGB(ExpandOneLetterByte((tempcolor & 0xF000) >> 12), ExpandOneLetterByte((tempcolor & 0x0F00) >> 8), ExpandOneLetterByte((tempcolor & 0x00F0) >> 4));
				picker->SetAlpha(ExpandOneLetterByte(tempcolor & 0x000F));
			}
			else if (text.length() == 6)
			{
				picker->SetRGB((tempcolor & 0xFF0000) >> 16, (tempcolor & 0x00FF00) >> 8, tempcolor & 0x0000FF);
			}
			else if (text.length() == 3)
			{
				picker->SetRGB(ExpandOneLetterByte((tempcolor & 0xF00) >> 8), ExpandOneLetterByte((tempcolor & 0x0F0) >> 4), ExpandOneLetterByte(tempcolor & 0x00F));
			}
			else
			{
				FailedParse(hDlg);
			}
		}
		catch (std::invalid_argument)
		{
			FailedParse(hDlg);
		}
	}
}

void CreateTarget(ID2D1Factory *factory, CComPtr<ID2D1HwndRenderTarget> &target, CComPtr<ID2D1SolidColorBrush> &brush, HWND hDlg, int item)
{
	const HWND item_handle = GetDlgItem(hDlg, item);
	RECT rect;
	GetClientRect(item_handle, &rect);
	target.Release();
	factory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(item_handle, D2D1::SizeU(rect.right, rect.bottom)), &target);

	brush.Release();
	target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &brush);
}