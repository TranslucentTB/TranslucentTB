#include "gui.hpp"
#include <algorithm>
#include <dwmapi.h>
#include <stdio.h>
#include <string>

#include "boolguard.hpp"
#include "subclasses/basetransparentsubclass.h"
#include "subclasses/nooutlinebuttonsubclass.h"

const Util::string_view_map<const uint32_t> GUI::COLOR_MAP = {
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

const std::tuple<const unsigned int, const unsigned int, const unsigned int> GUI::SLIDERS[] = {
	{ IDC_RED,        IDC_RSLIDER,   255 },
	{ IDC_GREEN,      IDC_GSLIDER,   255 },
	{ IDC_BLUE,       IDC_BSLIDER,   255 },
	{ IDC_ALPHA,      IDC_ASLIDER,   255 },
	{ IDC_HUE,        IDC_HSLIDER,   359 },
	{ IDC_SATURATION, IDC_SSLIDER,   100 },
	{ IDC_VALUE,      IDC_VSLIDER,   100 },
	{ IDC_HEXCOL,     IDC_HEXSLIDER, 0xFFFFFFFF }
};

std::unordered_map<const COLORREF *, HWND> GUI::m_pickerMap;

INT_PTR GUI::ColourPickerDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	GUI *const gui_data = reinterpret_cast<GUI *>(GetWindowLongPtr(hDlg, DWLP_USER));
	if (!gui_data && uMsg != WM_INITDIALOG)
	{
		return 0;
	}

	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		initdialog_pair_t *const init_pair = reinterpret_cast<initdialog_pair_t *>(lParam);
		SetWindowLongPtr(hDlg, DWLP_USER, reinterpret_cast<LONG_PTR>(init_pair->first));

		m_pickerMap[init_pair->second] = hDlg;

		return init_pair->first->OnDialogInit(hDlg);
	}

	case WM_DPICHANGED:
		return gui_data->OnDpiChange(hDlg);

	case WM_SIZE:
		return gui_data->OnSizeChange(hDlg);

	case WM_DRAWITEM:
		return gui_data->OnDrawItem(hDlg, lParam);

	case WM_ERASEBKGND:
		return gui_data->OnEraseBackground(hDlg, wParam);

	case WM_LBUTTONDOWN:
		return gui_data->OnClick(hDlg, lParam);

	case WM_MOUSEMOVE:
		return gui_data->OnMouseMove(hDlg, wParam);

	case WM_COMMAND:
		return gui_data->OnCommand(hDlg, wParam);

	case WM_NOTIFY:
		return gui_data->OnNotify(hDlg, lParam);

	case WM_NCCALCSIZE:
		return gui_data->OnNonClientCalculateSize(hDlg);

	case WM_DESTROY:
		return gui_data->OnWindowDestroy();

	default:
		return 0;
	}
}

INT_PTR GUI::OnDialogInit(HWND hDlg)
{
	// Set sliders buddies and ranges
	for (const auto &[buddy_id, slider_id, slider_max] : SLIDERS)
	{
		SendDlgItemMessage(hDlg, slider_id, UDM_SETBUDDY, reinterpret_cast<WPARAM>(GetDlgItem(hDlg, buddy_id)), 0);
		SendDlgItemMessage(hDlg, slider_id, UDM_SETRANGE32, 0, slider_max);

		SetWindowSubclass(GetDlgItem(hDlg, slider_id), BaseTransparentSubclass, slider_id, NULL);
	}

	InitHexInput(hDlg);

	// Set subclasses
	for (const int &button : { IDC_R, IDC_G, IDC_B, IDC_H, IDC_S, IDC_V })
	{
		SetWindowSubclass(GetDlgItem(hDlg, button), NoOutlineButtonSubclass, button, NULL);
	}

	// Set edit control text
	UpdateValues(hDlg);

	// Check red by default
	SendDlgItemMessage(hDlg, IDC_R, BM_SETCHECK, BST_CHECKED, 0);

	// Add tips for old & new color
	m_oldColorTip = CreateTip(hDlg, IDC_OLDCOLOR);
	m_newColorTip = CreateTip(hDlg, IDC_NEWCOLOR);

	// Calculate window position
	RECT coords;
	UINT flags = SWP_FRAMECHANGED | SWP_NOOWNERZORDER;
	HRESULT hr = CalculateDialogCoords(hDlg, coords);
	if (FAILED(hr))
	{
		GetClientRect(hDlg, &coords);
		flags |= SWP_NOMOVE;
	}

	const int x = coords.left;
	const int y = coords.top;
	const int width = coords.right - coords.left;
	const int height = coords.bottom - coords.top;

	// Extend DWM frame by 1 pixel for shadows
	//const MARGINS mar = { 1 };
	//DwmExtendFrameIntoClientArea(hDlg, &mar);

	SetWindowPos(hDlg, HWND_TOPMOST, x, y, width, height, flags);

	// Disable auto resize because it is wrong with a custom non-client area, handle it ourself in WM_DPICHANGE
	SetDialogDpiChangeBehavior(hDlg, DDC_DISABLE_RESIZE, DDC_DISABLE_RESIZE);

	// Initialize contexts and first draw
	const SColour &col = m_picker->GetCurrentColour();
	for (auto &[context, item_id] : m_contextPairs)
	{
		const HWND item_handle = GetDlgItem(hDlg, item_id);
		hr = context.Refresh(item_handle);
		if (FAILED(hr))
		{
			EndDialog(hDlg, hr);
			return 0;
		}

		hr = context.Draw(hDlg, col);
		if (FAILED(hr))
		{
			EndDialog(hDlg, hr);
			return 0;
		}
	}

	m_initDone = true;

	return 0;
}

INT_PTR GUI::OnDpiChange(HWND hDlg)
{
	if (m_initDone)
	{
		for (auto &[context, item_id] : m_contextPairs)
		{
			const HRESULT hr = context.OnDpiChange(GetDlgItem(hDlg, item_id));
			if (FAILED(hr))
			{
				EndDialog(hDlg, hr);
				return 0;
			}
		}

		RECT size = DllData::GetDialogSize();
		MapDialogRect(hDlg, &size);
		SetWindowPos(
			hDlg,
			nullptr,
			0,
			0,
			size.right - size.left,
			size.bottom - size.top,
			SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOMOVE
		);
	}

	return 0;
}

INT_PTR GUI::OnSizeChange(HWND hDlg)
{
	if (m_initDone)
	{
		for (auto &[context, item_id] : m_contextPairs)
		{
			const HRESULT hr = context.OnSizeChange(GetDlgItem(hDlg, item_id));
			if (FAILED(hr))
			{
				EndDialog(hDlg, hr);
				return 0;
			}
		}
	}

	return 0;
}

INT_PTR GUI::OnDrawItem(HWND hDlg, LPARAM lParam)
{
	if (m_initDone)
	{
		RenderContext *context;
		switch (reinterpret_cast<const DRAWITEMSTRUCT *>(lParam)->CtlID)
		{
		case IDC_COLORCIRCLE:
			context = &m_circleContext;
			break;

		case IDC_COLOR:
			context = &m_pickerContext;
			break;

		case IDC_COLOR2:
			context = &m_colorSliderContext;
			break;

		case IDC_ALPHASLIDE:
			context = &m_alphaSliderContext;
			break;

		case IDC_OLDCOLOR:
			context = &m_oldPreviewContext;
			break;

		case IDC_NEWCOLOR:
			context = &m_newPreviewContext;
			break;

		default: return 0;
		}

		const HRESULT hr = DrawItem(hDlg, *context, m_picker->GetCurrentColour());
		if (FAILED(hr))
		{
			EndDialog(hDlg, hr);
			return 0;
		}
	}

	return 1;
}

INT_PTR GUI::OnEraseBackground(HWND hDlg, WPARAM wParam)
{
	RECT rect;
	GetClientRect(hDlg, &rect);

	FillRect(
		reinterpret_cast<HDC>(wParam),
		&rect,
		reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH))
	);

	return true;
}

INT_PTR GUI::OnClick(HWND hDlg, LPARAM lParam)
{
	POINT p;
	GetCursorPos(&p);

	RECT rect;
	if (GetWindowRect(GetDlgItem(hDlg, IDC_COLOR), &rect); PtInRect(&rect, p))
	{
		OnColorPickerClick(hDlg, rect, p);
	}
	else if (GetWindowRect(GetDlgItem(hDlg, IDC_COLOR2), &rect); PtInRect(&rect, p))
	{
		OnColorSliderClick(hDlg, rect, p);
	}
	else if (GetWindowRect(GetDlgItem(hDlg, IDC_ALPHASLIDE), &rect); PtInRect(&rect, p))
	{
		OnAlphaSliderClick(hDlg, rect, p);
	}
	else if (lParam != NULL)
	{
		SetWindowLongPtr(hDlg, DWLP_MSGRESULT, DefWindowProc(hDlg, WM_NCLBUTTONDOWN, HTCAPTION, lParam));
		return 1;
	}

	return 0;
}

void GUI::OnColorPickerClick(HWND hDlg, RECT position, POINT cursor)
{
	const float fx = ((cursor.x - position.left) / static_cast<float>(position.right - position.left)) * 255.0f;
	const float fy = ((cursor.y - position.top) / static_cast<float>(position.bottom - position.top)) * 255.0f;
	const SColour &col = m_picker->GetCurrentColour();

	bool sliderRefresh = true;
	if (IsDlgButtonChecked(hDlg, IDC_R) == BST_CHECKED)
	{
		m_picker->SetRGB(col.r, fx, fy);
	}
	else if (IsDlgButtonChecked(hDlg, IDC_G) == BST_CHECKED)
	{
		m_picker->SetRGB(fx, col.g, fy);
	}
	else if (IsDlgButtonChecked(hDlg, IDC_B) == BST_CHECKED)
	{
		m_picker->SetRGB(fy, fx, col.b);
	}
	else if (IsDlgButtonChecked(hDlg, IDC_H) == BST_CHECKED)
	{
		m_picker->SetHSV(col.h, fx / 255.0f * 100.0f, (255 - fy) / 255.0f * 100.0f);
		sliderRefresh = false;
	}
	else if (IsDlgButtonChecked(hDlg, IDC_S) == BST_CHECKED)
	{
		m_picker->SetHSV(fx / 255.0f * 359.0f, col.s, (255 - fy) / 255.0f * 100.0f);
	}
	else if (IsDlgButtonChecked(hDlg, IDC_V) == BST_CHECKED)
	{
		m_picker->SetHSV(fx / 255.0f * 359.0f, (255 - fy) / 255.0f * 100.0f, col.v);
	}

	const HRESULT hr = Redraw(hDlg, true, false, !sliderRefresh);
	if (FAILED(hr))
	{
		EndDialog(hDlg, hr);
	}
}

void GUI::OnColorSliderClick(HWND hDlg, RECT position, POINT cursor)
{
	const float fy = ((cursor.y - position.top) / static_cast<float>(position.bottom - position.top)) * 255.0f;
	const SColour &col = m_picker->GetCurrentColour();

	if (IsDlgButtonChecked(hDlg, IDC_R) == BST_CHECKED)
	{
		m_picker->SetRGB(255 - fy, col.g, col.b);
	}
	else if (IsDlgButtonChecked(hDlg, IDC_G) == BST_CHECKED)
	{
		m_picker->SetRGB(col.r, 255 - fy, col.b);
	}
	else if (IsDlgButtonChecked(hDlg, IDC_B) == BST_CHECKED)
	{
		m_picker->SetRGB(col.r, col.g, 255 - fy);
	}
	else if (IsDlgButtonChecked(hDlg, IDC_H) == BST_CHECKED)
	{
		m_picker->SetHSV((255 - fy) / 255.0f * 359.0f, col.s, col.v);
	}
	else if (IsDlgButtonChecked(hDlg, IDC_S) == BST_CHECKED)
	{
		m_picker->SetHSV(col.h, (255 - fy) / 255.0f * 100.0f, col.v);
	}
	else if (IsDlgButtonChecked(hDlg, IDC_V) == BST_CHECKED)
	{
		m_picker->SetHSV(col.h, col.s, (255 - fy) / 255.0f * 100.0f);
	}

	const HRESULT hr = Redraw(hDlg);
	if (FAILED(hr))
	{
		EndDialog(hDlg, hr);
	}
}

void GUI::OnAlphaSliderClick(HWND hDlg, RECT position, POINT cursor)
{
	const float fy = ((cursor.y - position.top) / static_cast<float>(position.bottom - position.top)) * 255.0f;
	m_picker->SetAlpha(255 - fy);

	const HRESULT hr = Redraw(hDlg, true, true, true);
	if (FAILED(hr))
	{
		EndDialog(hDlg, hr);
	}
}

INT_PTR GUI::OnMouseMove(HWND hDlg, WPARAM wParam)
{
	if (wParam == MK_LBUTTON)
	{
		return OnClick(hDlg, NULL);
	}
	else
	{
		return 0;
	}
}

INT_PTR GUI::OnCommand(HWND hDlg, WPARAM wParam)
{
	switch (HIWORD(wParam))
	{
	case EN_SETFOCUS:
		return OnEditControlFocusAcquire(hDlg, wParam);

	case EN_KILLFOCUS:
		return OnEditControlFocusLoss(hDlg, wParam);

	case EN_CHANGE:
		return OnEditControlTextChange(hDlg, wParam);

	case BN_CLICKED:
		return OnButtonClick(hDlg, wParam);

	default:
		return 0;
	}
}

INT_PTR GUI::OnEditControlFocusAcquire(HWND hDlg, WPARAM wParam)
{
	if (LOWORD(wParam) != IDC_HEXCOL)
	{
		if (GetDlgItemInt(hDlg, LOWORD(wParam), NULL, FALSE) == 0)
		{
			SetDlgItemText(hDlg, LOWORD(wParam), L"");
		}
	}
	SendDlgItemMessage(hDlg, LOWORD(wParam), EM_SETSEL, 0, -1);
	return 0;
}

INT_PTR GUI::OnEditControlFocusLoss(HWND hDlg, WPARAM wParam)
{
	if (LOWORD(wParam) == IDC_HEXCOL)
	{
		ParseHex(hDlg);
		const HRESULT hr = Redraw(hDlg);
		if (FAILED(hr))
		{
			EndDialog(hDlg, hr);
		}

		return 0;
	}

	bool_guard guard(m_changingText);
	for (const auto &[buddy_id, slider_id, _] : SLIDERS)
	{
		if (buddy_id == IDC_HEXCOL)
		{
			const SColour &col = m_picker->GetCurrentColour();

			wchar_t buff[11];
			_snwprintf_s(buff, 10, L"0x%02X%02X%02X%02X", col.r, col.g, col.b, col.a);

			SetDlgItemText(hDlg, buddy_id, buff);
		}
		else
		{
			BOOL result;
			SetDlgItemInt(hDlg, buddy_id, SendDlgItemMessage(hDlg, slider_id, UDM_GETPOS, 0, reinterpret_cast<LPARAM>(&result)), false);
		}
	}

	return 0;
}

INT_PTR GUI::OnEditControlTextChange(HWND hDlg, WPARAM wParam)
{
	if (m_changingText)
	{
		return 0;
	}

	BOOL result;
	switch (LOWORD(wParam))
	{
	case IDC_RED:
	case IDC_GREEN:
	case IDC_BLUE:
		m_picker->SetRGB(SendDlgItemMessage(hDlg, IDC_RSLIDER, UDM_GETPOS, 0, reinterpret_cast<LPARAM>(&result)), SendDlgItemMessage(hDlg, IDC_GSLIDER, UDM_GETPOS, 0, reinterpret_cast<LPARAM>(&result)), SendDlgItemMessage(hDlg, IDC_BSLIDER, UDM_GETPOS, 0, reinterpret_cast<LPARAM>(&result)));
		break;

	case IDC_HUE:
	case IDC_SATURATION:
	case IDC_VALUE:
		m_picker->SetHSV(SendDlgItemMessage(hDlg, IDC_HSLIDER, UDM_GETPOS, 0, reinterpret_cast<LPARAM>(&result)), SendDlgItemMessage(hDlg, IDC_SSLIDER, UDM_GETPOS, 0, reinterpret_cast<LPARAM>(&result)), SendDlgItemMessage(hDlg, IDC_VSLIDER, UDM_GETPOS, 0, reinterpret_cast<LPARAM>(&result)));
		break;

	case IDC_ALPHA:
		m_picker->SetAlpha(SendDlgItemMessage(hDlg, IDC_ASLIDER, UDM_GETPOS, 0, reinterpret_cast<LPARAM>(&result)));
		break;

	case IDC_HEXCOL:
		if (!m_changingHexViaSpin)
		{
			return 0;
		}
		else
		{
			ParseHex(hDlg);
			m_changingHexViaSpin = false;
		}
	}

	const HRESULT hr = Redraw(hDlg);
	if (FAILED(hr))
	{
		EndDialog(hDlg, hr);
	}

	return 0;
}

INT_PTR GUI::OnButtonClick(HWND hDlg, WPARAM wParam)
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
		const HRESULT hr = Redraw(hDlg, false, false, false, true, true, false);
		if (FAILED(hr))
		{
			EndDialog(hDlg, hr);
		}
		return 0;
	}

	case IDC_NEWCOLOR:
	{
		const SColour &col = m_picker->GetCurrentColour();

		m_picker->SetRGB(255 - col.r, 255 - col.g, 255 - col.b);
		const HRESULT hr = Redraw(hDlg);
		if (FAILED(hr))
		{
			EndDialog(hDlg, hr);
		}
		return 0;
	}

	case IDC_OLDCOLOR:
	{
		const SColour &old = m_picker->GetOldColour();

		m_picker->SetRGB(old.r, old.g, old.b);
		m_picker->SetAlpha(old.a);
		const HRESULT hr = Redraw(hDlg);
		if (FAILED(hr))
		{
			EndDialog(hDlg, hr);
		}
		return 0;
	}

	case IDOK:
		m_picker->UpdateOldColour();
		EndDialog(hDlg, 0xfff);
		return 0;

	case IDCANCEL:
	{
		const SColour &old = m_picker->GetOldColour();

		m_picker->SetRGB(old.r, old.g, old.b);
		m_picker->SetAlpha(old.a);
		EndDialog(hDlg, 0xfff);
		return 0;
	}

	default:
		return 0;
	}
}

INT_PTR GUI::OnNotify(HWND hDlg, LPARAM lParam)
{
	NMHDR &notify = *reinterpret_cast<NMHDR *>(lParam);
	switch (notify.code)
	{
	case UDN_DELTAPOS:
		return OnUpDownControlChange(notify);

	case TTN_GETDISPINFO:
		return OnEditControlRequestWatermarkInfo(hDlg, notify);

	default:
		return 0;
	}
}

INT_PTR GUI::OnUpDownControlChange(NMHDR notify)
{
	if (notify.idFrom == IDC_HEXSLIDER)
	{
		m_changingHexViaSpin = true;
	}

	return 0;
}

INT_PTR GUI::OnEditControlRequestWatermarkInfo(HWND hDlg, NMHDR &notify)
{
	NMTTDISPINFO &dispinfo = reinterpret_cast<NMTTDISPINFO &>(notify);
	if (notify.idFrom == reinterpret_cast<UINT_PTR>(GetDlgItem(hDlg, IDC_NEWCOLOR)))
	{
		wcscpy_s(dispinfo.szText, L"Click to invert color");
	}
	else if (notify.idFrom == reinterpret_cast<UINT_PTR>(GetDlgItem(hDlg, IDC_OLDCOLOR)))
	{
		wcscpy_s(dispinfo.szText, L"Click to restore old color");
	}
	return 0;
}

INT_PTR GUI::OnNonClientCalculateSize(HWND hDlg)
{
	SetWindowLongPtr(hDlg, DWLP_MSGRESULT, 0);
	return 1;
}

INT_PTR GUI::OnWindowDestroy()
{
	DestroyWindow(m_oldColorTip);
	DestroyWindow(m_newColorTip);

	return 0;
}

void GUI::InitHexInput(HWND hDlg)
{
	// Slider in base 16
	SendDlgItemMessage(hDlg, IDC_HEXSLIDER, UDM_SETBASE, 16, 0);

	// Set maximum input length
	static const auto longestName = std::max_element(COLOR_MAP.begin(), COLOR_MAP.end(), [](auto &&a, auto &&b)
	{
		return a.first.length() < b.first.length();
	})->first.length();
	SendDlgItemMessage(hDlg, IDC_HEXCOL, EM_SETLIMITTEXT, longestName, 0);

	// Watermark
	Edit_SetCueBannerTextFocused(GetDlgItem(hDlg, IDC_HEXCOL), L"CSS color name or hex", TRUE);
}

HWND GUI::CreateTip(HWND hDlg, int item)
{
	HWND tip = CreateWindowEx(
		WS_EX_TOPMOST,
		TOOLTIPS_CLASS,
		NULL,
		TTS_ALWAYSTIP,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		hDlg,
		NULL,
		DllData::GetInstanceHandle(),
		NULL
	);

	TOOLINFO ti = {
		sizeof(ti),
		TTF_IDISHWND | TTF_SUBCLASS,
		hDlg,
		reinterpret_cast<UINT_PTR>(GetDlgItem(hDlg, item))
	};
	ti.lpszText = LPSTR_TEXTCALLBACK;
	SendMessage(tip, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&ti));
	SendMessage(tip, TTM_ACTIVATE, TRUE, 0);

	return tip;
}

HRESULT GUI::CalculateDialogCoords(HWND hDlg, RECT &coords)
{
	POINT point;
	if (!GetCursorPos(&point))
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	HMONITOR mon = MonitorFromPoint(point, MONITOR_DEFAULTTONEAREST);
	MONITORINFO mi = { sizeof(mi) };
	if (!GetMonitorInfo(mon, &mi))
	{
		return E_FAIL;
	}

	// Move to the appropriate monitor to consider DPI.
	// Window still invisible at this point.
	BOOL ret = SetWindowPos(
		hDlg,
		nullptr,
		mi.rcWork.right,
		mi.rcWork.top,
		0,
		0,
		SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOSIZE
	);

	if (!ret)
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	RECT rect;
	if (!GetClientRect(hDlg, &rect))
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	const long width = rect.right - rect.left;
	const long height = rect.bottom - rect.top;

	// First try centering it on the mouse
	coords.left = point.x - (width / 2);
	coords.right = coords.left + width;

	coords.top = point.y - (height / 2);
	coords.bottom = coords.top + height;

	if (RectFitsInRect(mi.rcWork, coords))
	{
		return S_OK; // It fits!
	}

	const bool rightDoesntFits = coords.right > mi.rcWork.right;
	const bool leftDoesntFits = coords.left < mi.rcWork.left;
	const bool bottomDoesntFits = coords.bottom > mi.rcWork.bottom;
	const bool topDoesntFits = coords.top < mi.rcWork.top;

	if ((rightDoesntFits && leftDoesntFits) || (bottomDoesntFits && topDoesntFits))
	{
		// Doesn't fits in the monitor work area (lol wat)
		return E_BOUNDS;
	}

	// Offset the rect so that it is completely in the work area
	int x_offset = 0;
	if (rightDoesntFits)
	{
		x_offset = mi.rcWork.right - coords.right; // Negative offset
	}
	else if (leftDoesntFits)
	{
		x_offset = mi.rcWork.left - coords.left;
	}

	int y_offset = 0;
	if (bottomDoesntFits)
	{
		y_offset = mi.rcWork.bottom - coords.bottom; // Negative offset
	}
	else if (topDoesntFits)
	{
		y_offset = mi.rcWork.top - coords.top;
	}

	if (!OffsetRect(&coords, x_offset, y_offset))
	{
		return E_FAIL;
	}

	// Shouldn't return false at this point, but still check
	if (!RectFitsInRect(mi.rcWork, coords))
	{
		return E_BOUNDS;
	}

	return S_OK;
}

HRESULT GUI::Redraw(HWND hDlg, bool skipMain, bool skipCircle, bool skipSlide, bool skipAlpha, bool skipNew, bool updateValues)
{
	if (updateValues)
	{
		UpdateValues(hDlg);
	}
	RedrawWindow(hDlg, NULL, NULL, RDW_UPDATENOW);

	const SColourF col = m_picker->GetCurrentColour();
	HRESULT hr;

	if (!skipMain)
	{
		hr = DrawItem(hDlg, m_pickerContext, col);
		if (FAILED(hr))
		{
			return hr;
		}
	}

	if (!skipCircle)
	{
		hr = DrawItem(hDlg, m_circleContext, col);
		if (FAILED(hr))
		{
			return hr;
		}
	}

	if (!skipSlide)
	{
		hr = DrawItem(hDlg, m_colorSliderContext, col);
		if (FAILED(hr))
		{
			return hr;
		}
	}

	if (!skipAlpha)
	{
		hr = DrawItem(hDlg, m_alphaSliderContext, col);
		if (FAILED(hr))
		{
			return hr;
		}
	}

	if (!skipNew)
	{
		hr = DrawItem(hDlg, m_newPreviewContext, col);
		if (FAILED(hr))
		{
			return hr;
		}
	}

	return S_OK;
}

HRESULT GUI::DrawItem(HWND hDlg, RenderContext &context, const SColourF &col)
{
	HRESULT hr = context.Draw(hDlg, col);
	if (FAILED(hr))
	{
		for (auto &[context2, item_id] : m_contextPairs)
		{
			const HWND item_handle = GetDlgItem(hDlg, item_id);
			hr = context2.Refresh(item_handle);
			if (FAILED(hr))
			{
				return hr;
			}

			hr = context2.Draw(hDlg, col);
			if (FAILED(hr))
			{
				return hr;
			}
		}
	}

	return S_OK;
}

GUI::GUI(CColourPicker *picker, ID2D1Factory3 *factory, IDWriteFactory *dwFactory) :
	m_picker(picker),
	m_pickerContext(factory),
	m_circleContext(factory),
	m_colorSliderContext(factory),
	m_alphaSliderContext(factory),
	m_oldPreviewContext(picker->GetOldColour(), factory, dwFactory),
	m_newPreviewContext(factory, dwFactory),
	m_contextPairs
	{
		{ m_pickerContext, IDC_COLOR },
		{ m_circleContext, IDC_COLORCIRCLE },
		{ m_colorSliderContext, IDC_COLOR2 },
		{ m_alphaSliderContext, IDC_ALPHASLIDE },
		{ m_oldPreviewContext, IDC_OLDCOLOR },
		{ m_newPreviewContext, IDC_NEWCOLOR }
	},
	m_changingText(false),
	m_changingHexViaSpin(false),
	m_initDone(false),
	m_oldColorTip(nullptr),
	m_newColorTip(nullptr)
{ }

void GUI::UpdateValues(HWND hDlg)
{
	const SColour &col = m_picker->GetCurrentColour();
	bool_guard guard(m_changingText);

	SendDlgItemMessage(hDlg, IDC_RSLIDER, UDM_SETPOS, 0, col.r);
	SendDlgItemMessage(hDlg, IDC_GSLIDER, UDM_SETPOS, 0, col.g);
	SendDlgItemMessage(hDlg, IDC_BSLIDER, UDM_SETPOS, 0, col.b);
	SendDlgItemMessage(hDlg, IDC_ASLIDER, UDM_SETPOS, 0, col.a);
	SendDlgItemMessage(hDlg, IDC_HSLIDER, UDM_SETPOS, 0, col.h);
	SendDlgItemMessage(hDlg, IDC_SSLIDER, UDM_SETPOS, 0, col.s);
	SendDlgItemMessage(hDlg, IDC_VSLIDER, UDM_SETPOS, 0, col.v);
	SendDlgItemMessage(hDlg, IDC_HEXSLIDER, UDM_SETPOS32, 0, (col.r << 24) + (col.g << 16) + (col.b << 8) + col.a);
}

void GUI::ParseHex(HWND hDlg)
{
	std::wstring text;
	text.resize(21);
	int count = GetDlgItemText(hDlg, IDC_HEXCOL, text.data(), 21);
	text.resize(count);
	Util::TrimInplace(text);

	if (text.empty())
	{
		return;
	}
	else if (COLOR_MAP.count(text) != 0)
	{
		const uint32_t &color = COLOR_MAP.at(text);
		m_picker->SetRGB((color & 0xFF0000) >> 16, (color & 0x00FF00) >> 8, color & 0x0000FF);
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
			// Not using Util::ClampTo here, because if the user enters something too big, instead
			// of being pitch black, it'll act as if it truncated the value they entered.
			const unsigned int tempcolor = std::stoull(text, nullptr, 16) & 0xFFFFFFFF;

			if (text.length() == 8)
			{
				m_picker->SetRGB((tempcolor & 0xFF000000) >> 24, (tempcolor & 0x00FF0000) >> 16, (tempcolor & 0x0000FF00) >> 8);
				m_picker->SetAlpha(tempcolor & 0x000000FF);
			}
			else if (text.length() == 4)
			{
				m_picker->SetRGB(ExpandOneLetterByte((tempcolor & 0xF000) >> 12), ExpandOneLetterByte((tempcolor & 0x0F00) >> 8), ExpandOneLetterByte((tempcolor & 0x00F0) >> 4));
				m_picker->SetAlpha(ExpandOneLetterByte(tempcolor & 0x000F));
			}
			else if (text.length() == 6)
			{
				m_picker->SetRGB((tempcolor & 0xFF0000) >> 16, (tempcolor & 0x00FF00) >> 8, tempcolor & 0x0000FF);
			}
			else if (text.length() == 3)
			{
				m_picker->SetRGB(ExpandOneLetterByte((tempcolor & 0xF00) >> 8), ExpandOneLetterByte((tempcolor & 0x0F0) >> 4), ExpandOneLetterByte(tempcolor & 0x00F));
			}
			else
			{
				FailedParse(hDlg);
			}
		}
		catch (...)
		{
			FailedParse(hDlg);
		}
	}
}

void GUI::FailedParse(HWND hDlg)
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

HRESULT GUI::CreateGUI(CColourPicker *picker, COLORREF &value, HWND hParent)
{
	if (m_pickerMap.count(&value) == 0)
	{
		const D2D1_FACTORY_OPTIONS fo = {
#ifdef _DEBUG
			D2D1_DEBUG_LEVEL_INFORMATION
#endif
		};

		winrt::com_ptr<ID2D1Factory3> factory;
		HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, fo, factory.put());
		if (FAILED(hr))
		{
			return hr;
		}

		winrt::com_ptr<IDWriteFactory> dwFactory;
		hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
			__uuidof(IDWriteFactory),
			reinterpret_cast<IUnknown **>(dwFactory.put())
		);
		if (FAILED(hr))
		{
			return hr;
		}

		GUI gui_inst(picker, factory.get(), dwFactory.get());
		initdialog_pair_t init_pair(&gui_inst, &value);
		INT_PTR result = DialogBoxParam(DllData::GetInstanceHandle(), MAKEINTRESOURCE(IDD_COLORPICKER), hParent, ColourPickerDlgProc, reinterpret_cast<LPARAM>(&init_pair));
		m_pickerMap.erase(&value);
		if (result == 0)
		{
			return HRESULT_FROM_WIN32(ERROR_INVALID_WINDOW_HANDLE);
		}
		else if (result == -1)
		{
			return HRESULT_FROM_WIN32(GetLastError());
		}
		else if (result == 0xfff)
		{
			return S_OK;
		}
		else
		{
			return static_cast<HRESULT>(result);
		}
	}
	else
	{
		SetForegroundWindow(m_pickerMap.at(&value));
		return S_OK;
	}
}