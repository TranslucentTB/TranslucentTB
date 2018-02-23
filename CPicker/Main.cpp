#include <chrono>
#include <future>
#include <stdio.h>

#include "CPicker.h"
#include "resource.h"

static bool programmaticallyChangingText;

LRESULT CALLBACK ColourPickerDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static CColourPicker *picker;
	static PixelBuffer pbufferC1;
	static PixelBuffer pbufferC2;
	static PixelBuffer pbufferA;
	static bool open;

	const HWND Color1 = GetDlgItem(hDlg, IDC_COLOR);
	const HWND Color2 = GetDlgItem(hDlg, IDC_COLOR2);
	const HWND Alpha = GetDlgItem(hDlg, IDC_ALPHASLIDE);

	RECT rectC1;
	RECT rectC2;
	RECT rectA;

	GetWindowRect(Color1, &rectC1);
	GetWindowRect(Color2, &rectC2);
	GetWindowRect(Alpha, &rectA);

	const float widthC1 = rectC1.right - rectC1.left;
	const float heightC1 = rectC1.bottom - rectC1.top;

	const float widthC2 = rectC2.right - rectC2.left;
	const float heightC2 = rectC2.bottom - rectC2.top;

	const float widthA = rectA.right - rectA.left;
	const float heightA = rectA.bottom - rectA.top;

	const int red = GetDlgItemInt(hDlg, IDC_RED, NULL, false);
	const int green = GetDlgItemInt(hDlg, IDC_GREEN, NULL, false);
	const int blue = GetDlgItemInt(hDlg, IDC_BLUE, NULL, false);

	const int hue = GetDlgItemInt(hDlg, IDC_HUE, NULL, false);
	const int saturation = GetDlgItemInt(hDlg, IDC_SATURATION, NULL, false);
	const int value = GetDlgItemInt(hDlg, IDC_VALUE, NULL, false);

	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		picker = (CColourPicker*)lParam;
		pbufferC1.Create(widthC1, heightC1);
		pbufferC2.Create(widthC2, heightC2);
		pbufferA.Create(widthA, heightA);

		UpdateValues(hDlg, picker->GetCurrentColour());

		for (int item : {IDC_RED, IDC_GREEN, IDC_BLUE, IDC_ALPHA, IDC_HUE, IDC_SATURATION, IDC_VALUE})
		{
			SendDlgItemMessage(hDlg, item, EM_SETLIMITTEXT, 3, 0);
		}

		SendDlgItemMessage(hDlg, IDC_R, BM_SETCHECK, BST_CHECKED, 0);

		open = true;
		programmaticallyChangingText = false;
		std::thread(
			[hDlg]()
			{
				while (open)
				{
					SendMessage(hDlg, WM_DPICHANGED, 0, 0);
					std::this_thread::sleep_for(std::chrono::seconds(1));
				}
			}
		).detach();

		break;
	}

	case WM_DPICHANGED:
		pbufferC1.Destroy();
		pbufferC2.Destroy();
		pbufferA.Destroy();
		pbufferC1.Create(widthC1, heightC1);
		pbufferC2.Create(widthC2, heightC2);
		pbufferA.Create(widthA, heightA);
	case WM_PAINT:
	{
		static bool can_run = true;
		if (!can_run)
		{
			break;
		}

		const int FPS = 30;
		const int frame_time = 1000000 / FPS;
		auto start_time = std::chrono::high_resolution_clock::now();

		const HWND CurrentColor = GetDlgItem(hDlg, IDC_CURRCOLOR);
		const HWND OldColor = GetDlgItem(hDlg, IDC_OLDCOLOR);

		HDC hdc, hcomp;
		HBITMAP hbmp;
		const HBRUSH backgroundColorBrush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
		LOGBRUSH backgroundColor;
		GetObject(backgroundColorBrush, sizeof(backgroundColor), &backgroundColor);
		float rf, gf, bf;

		// Big color selector (displays non-selected features)
		// For example, if G is selected (in RGB radio box) it displays red vs. blue colors
		hdc = GetDC(Color1);
		hcomp = CreateCompatibleDC(hdc);

		hbmp = CreateCompatibleBitmap(hdc, widthC1, heightC1);
		SelectObject(hcomp, hbmp);

		// Check who is selected.
		// RED
		if (IsDlgButtonChecked(hDlg, IDC_R) == BST_CHECKED)
		{
			for (int g = 0; g < widthC1; g++)
			{
				gf = (g / widthC1) * 255.0f;
				for (int b = 0; b < heightC1; b++)
				{
					bf = (b / heightC1) * 255.0f;
					pbufferC1.SetPixel(g, b, RGB(red, gf, bf));
				}
			}

			pbufferC1.Display(hcomp);
			DrawCircle(hcomp, red, green, blue, (green / 255.0f) * widthC1, (blue / 255.0f) * heightC1);
		}

		// GREEN
		else if (IsDlgButtonChecked(hDlg, IDC_G) == BST_CHECKED)
		{
			for (int r = 0; r < widthC1; r++)
			{
				rf = (r / widthC1) * 255.0f;
				for (int b = 0; b < heightC1; b++)
				{
					bf = (b / heightC1) * 255.0f;
					pbufferC1.SetPixel(r, b, RGB(rf, green, bf));
				}
			}
			pbufferC1.Display(hcomp);
			DrawCircle(hcomp, red, green, blue, (red / 255.0f) * widthC1, (blue / 255.0f) * heightC1);
		}

		// BLUE
		else if (IsDlgButtonChecked(hDlg, IDC_B) == BST_CHECKED)
		{
			for (int g = 0; g < widthC1; g++)
			{
				gf = (g / widthC1) * 255.0f;
				for (int r = 0; r < heightC1; r++)
				{
					rf = (r / heightC1) * 255.0f;
					pbufferC1.SetPixel(g, r, RGB(rf, gf, blue));
				}
			}
			pbufferC1.Display(hcomp);
			DrawCircle(hcomp, red, green, blue, (green / 255.0f) * widthC1, (red / 255.0f) * heightC1);
		}

		// HUE
		else if (IsDlgButtonChecked(hDlg, IDC_H) == BST_CHECKED)
		{
			double sat, val, stepsat, stepval;
			SColour tempcol;

			sat = val = 0.0;
			stepsat = 100.0 / widthC1;
			stepval = 100.0 / heightC1;

			tempcol.h = hue;
			tempcol.s = sat;
			tempcol.v = val;

			for (int y = heightC1 - 1; y > -1; y--)
			{
				for (int x = 0; x < widthC1; x++)
				{
					sat += stepsat;
					tempcol.s = sat;
					tempcol.UpdateRGB();
					pbufferC1.SetPixel(x, y, RGB(tempcol.r, tempcol.g, tempcol.b));
				}

				val += stepval;
				sat = 0.0;
				tempcol.v = val;
			}
			pbufferC1.Display(hcomp);

			// Draws circle
			tempcol.s = saturation;
			tempcol.v = value;
			tempcol.UpdateRGB();

			DrawCircle(hcomp, tempcol.r, tempcol.g, tempcol.b, tempcol.s / stepsat, heightC1 - (tempcol.v / stepval));
		}

		// SATURATION
		else if (IsDlgButtonChecked(hDlg, IDC_S) == BST_CHECKED)
		{
			double temphue, val, stepval, stephue;
			SColour tempcol;

			temphue = val = 0.0;
			stephue = 359.0 / widthC1;
			stepval = 100.0 / heightC1;

			tempcol.h = temphue;
			tempcol.s = saturation;
			tempcol.v = val;

			for (int y = heightC1 - 1; y > -1; y--)
			{
				for (int x = 0; x < widthC1; x++)
				{
					temphue += stephue;
					tempcol.h = temphue;
					tempcol.UpdateRGB();
					pbufferC1.SetPixel(x, y, RGB(tempcol.r, tempcol.g, tempcol.b));
				}

				val += stepval;
				temphue = 0.0;
				tempcol.v = val;
			}
			pbufferC1.Display(hcomp);

			// Draws circle
			tempcol.h = hue;
			tempcol.v = value;
			tempcol.UpdateRGB();

			DrawCircle(hcomp, tempcol.r, tempcol.g, tempcol.b, tempcol.h / stephue, heightC1 - (tempcol.v / stepval));
		}

		// VALUE
		else if (IsDlgButtonChecked(hDlg, IDC_V) == BST_CHECKED)
		{
			double temphue, sat, stepsat, stephue;
			SColour tempcol;

			temphue = sat = 0.0;
			stephue = 359.0 / widthC1;
			stepsat = 100.0 / heightC1;

			tempcol.h = temphue;
			tempcol.s = sat;
			tempcol.v = value;

			for (int y = heightC1 - 1; y > -1; y--)
			{
				for (int x = 0; x < widthC1; x++)
				{
					temphue += stephue;
					tempcol.h = temphue;
					tempcol.UpdateRGB();
					pbufferC1.SetPixel(x, y, RGB(tempcol.r, tempcol.g, tempcol.b));
				}

				sat += stepsat;
				temphue = 0.0;
				tempcol.s = sat;
			}
			pbufferC1.Display(hcomp);

			// Draws circle
			tempcol.h = hue;
			tempcol.s = saturation;
			tempcol.UpdateRGB();

			DrawCircle(hcomp, tempcol.r, tempcol.g, tempcol.b, tempcol.h / stephue, heightC1 - (tempcol.s / stepsat));
		}

		BitBlt(hdc, 0, 0, widthC1, heightC1, hcomp, 0, 0, SRCCOPY);

		DeleteObject(hbmp);
		DeleteDC(hcomp);
		ReleaseDC(Color1, hdc);

		// Small color selector (displays selected feature)
		hdc = GetDC(Color2);
		hcomp = CreateCompatibleDC(hdc);

		hbmp = CreateCompatibleBitmap(hdc, widthC2, heightC2);
		SelectObject(hcomp, hbmp);

		for (int y = heightC2 - 1; y > -1; y--)
		{
			for (int x = 0; x < 6; x++)
			{
				pbufferC2.SetPixel(x, y, backgroundColor.lbColor);
			}
			for (int x = widthC2 - 6; x < widthC2; x++)
			{
				pbufferC2.SetPixel(x, y, backgroundColor.lbColor);
			}
		}

		// Check who is selected.
		// RED
		if (IsDlgButtonChecked(hDlg, IDC_R) == BST_CHECKED)
		{
			for (int r = heightC2 - 1; r > -1; r--)
			{
				rf = ((r / heightC2) * 255.0f);
				for (int x = 6; x < widthC2 - 6; x++)
				{
					pbufferC2.SetPixel(x, r, RGB(255 - rf, green, blue));
				}
			}
			pbufferC2.Display(hcomp);
			DrawArrows(hcomp, widthC2, heightC2, (red / 255.0f) * heightC2);
		}
		// GREEN
		else if (IsDlgButtonChecked(hDlg, IDC_G) == BST_CHECKED)
		{
			for (int g = heightC2 - 1; g > -1; g--)
			{
				gf = ((g / heightC2) * 255.0f);
				for (int x = 6; x < widthC2 - 6; x++)
				{
					pbufferC2.SetPixel(x, g, RGB(red, 255 - gf, blue));
				}
			}
			pbufferC2.Display(hcomp);
			DrawArrows(hcomp, widthC2, heightC2, (green / 255.0f) * heightC2);
		}
		// BLUE
		else if (IsDlgButtonChecked(hDlg, IDC_B) == BST_CHECKED)
		{
			for (int b = heightC2 - 1; b > -1; b--)
			{
				bf = ((b / heightC2) * 255.0f);
				for (int x = 6; x < widthC2 - 6; x++)
				{
					pbufferC2.SetPixel(x, b, RGB(red, green, 255 - bf));
				}
			}
			pbufferC2.Display(hcomp);
			DrawArrows(hcomp, widthC2, heightC2, (blue / 255.0f) * heightC2);
		}
		// HUE
		else if (IsDlgButtonChecked(hDlg, IDC_H) == BST_CHECKED)
		{
			double temphue, step;
			SColour tempcol;

			temphue = 0.0;

			tempcol.h = 0;
			tempcol.s = 100;
			tempcol.v = 100;

			step = 359.0 / heightC2;

			for (int y = heightC2 - 1; y > -1; y--)
			{
				tempcol.UpdateRGB();

				for (int x = 6; x < widthC2 - 6; x++)
				{
					pbufferC2.SetPixel(x, y, RGB(tempcol.r, tempcol.g, tempcol.b));
				}

				temphue += step;
				tempcol.h = temphue;
			}
			pbufferC2.Display(hcomp);

			temphue = GetDlgItemInt(hDlg, IDC_HUE, NULL, false);
			temphue = temphue / step;

			DrawArrows(hcomp, widthC2, heightC2, temphue);
		}
		// SATURATION
		else if (IsDlgButtonChecked(hDlg, IDC_S) == BST_CHECKED)
		{
			double sat, step;
			SColour tempcol;

			sat = 0.0;

			tempcol.h = GetDlgItemInt(hDlg, IDC_HUE, NULL, false);
			tempcol.s = 0;
			tempcol.v = GetDlgItemInt(hDlg, IDC_VALUE, NULL, false);

			step = 100.0 / heightC2;

			for (int y = heightC2 - 1; y > -1; y--)
			{
				tempcol.UpdateRGB();

				for (int x = 6; x < widthC2 - 6; x++)
				{
					pbufferC2.SetPixel(x, y, RGB(tempcol.r, tempcol.g, tempcol.b));
				}

				sat += step;
				tempcol.s = sat;
			}
			pbufferC2.Display(hcomp);

			sat = GetDlgItemInt(hDlg, IDC_SATURATION, NULL, false);
			sat = sat / step;

			DrawArrows(hcomp, widthC2, heightC2, sat);
		}
		// VALUE
		else if (IsDlgButtonChecked(hDlg, IDC_V) == BST_CHECKED)
		{
			double val, step;
			SColour tempcol;

			val = 0.0;

			tempcol.h = GetDlgItemInt(hDlg, IDC_HUE, NULL, false);
			tempcol.s = GetDlgItemInt(hDlg, IDC_SATURATION, NULL, false);
			tempcol.v = 0;

			step = 100.0 / heightC2;

			for (int y = heightC2 - 1; y > -1; y--)
			{
				tempcol.UpdateRGB();

				for (int x = 6; x < widthC2 - 6; x++)
				{
					pbufferC2.SetPixel(x, y, RGB(tempcol.r, tempcol.g, tempcol.b));
				}

				val += step;
				tempcol.v = val;
			}
			pbufferC2.Display(hcomp);

			val = GetDlgItemInt(hDlg, IDC_VALUE, NULL, false);
			val = val / step;

			DrawArrows(hcomp, widthC2, heightC2, val);
		}

		BitBlt(hdc, 0, 0, widthC2, heightC2, hcomp, 0, 0, SRCCOPY);

		DeleteObject(hbmp);
		DeleteDC(hcomp);
		ReleaseDC(Color2, hdc);

		// Alpha slider
		hdc = GetDC(Alpha);
		hcomp = CreateCompatibleDC(hdc);

		hbmp = CreateCompatibleBitmap(hdc, widthA, heightA);
		SelectObject(hcomp, hbmp);

		for (int y = heightA - 1; y > -1; y--)
		{
			for (int x = 0; x < 6; x++)
			{
				pbufferA.SetPixel(x, y, backgroundColor.lbColor);
			}
			for (int x = widthA - 6; x < widthA; x++)
			{
				pbufferA.SetPixel(x, y, backgroundColor.lbColor);
			}
		}

		rf = red / 255.0f;
		gf = green / 255.0f;
		bf = blue / 255.0f;
		bool flag = false;

		for (int y = heightA - 1; y > -1; y--)
		{
			COLORREF cb, cw;

			if (!(y % (int)(widthA / 2 - 6)))
			{
				flag = !flag;
			}

			float af = 1.0f - (y / heightA);

			cb = RGB((rf*af) * 255, (gf*af) * 255, (bf*af) * 255);
			cw = RGB((rf*af + 1 - af) * 255, (gf*af + 1 - af) * 255, (bf*af + 1 - af) * 255);

			for (int x = 6; x < (widthA / 2); x++)
			{
				pbufferA.SetPixel(x, y, flag ? cw : cb);
			}
			for (int x = (widthA / 2); x < widthA - 6; x++)
			{
				pbufferA.SetPixel(x, y, flag ? cb : cw);
			}
		}
		pbufferA.Display(hcomp);

		DrawArrows(hcomp, widthA, heightA, (GetDlgItemInt(hDlg, IDC_ALPHA, NULL, false) / 100.0f) * heightA);

		BitBlt(hdc, 0, 0, widthA, heightA, hcomp, 0, 0, SRCCOPY);

		DeleteObject(hbmp);
		DeleteDC(hcomp);
		ReleaseDC(Alpha, hdc);

		DeleteObject(backgroundColorBrush);


		DrawCheckedRect(CurrentColor, picker->GetCurrentColour().r, picker->GetCurrentColour().g,
			picker->GetCurrentColour().b, picker->GetCurrentColour().a, 10, 10);

		DrawCheckedRect(OldColor, picker->GetOldColour().r, picker->GetOldColour().g, picker->GetOldColour().b,
			picker->GetOldColour().a, 10, 10);

		auto end_time = std::chrono::high_resolution_clock::now();
		auto time = end_time - start_time;

		long long draw_time = std::chrono::duration_cast<std::chrono::microseconds>(time).count();
		if (draw_time < frame_time)
		{
			std::async(std::launch::async,
				[&draw_time]()
				{
					can_run = false;
					std::this_thread::sleep_for(std::chrono::microseconds(frame_time - draw_time));
					can_run = true;
				}
			);
		}

		break;
	}

	case WM_LBUTTONDOWN:
	case WM_MOUSEMOVE:
	{
		if (uMsg == WM_MOUSEMOVE && wParam != MK_LBUTTON)
		{
			break;
		}

		POINT p;
		GetCursorPos(&p);

		// IDC_COLOR1 picked
		if (_IS_IN(rectC1.left, rectC1.right, p.x) && _IS_IN(rectC1.top, rectC1.bottom, p.y))
		{
			const float fx = ((p.x - rectC1.left) / widthC1) * 255.0f;
			const float fy = ((p.y - rectC1.top) / heightC1) * 255.0f;

			if (IsDlgButtonChecked(hDlg, IDC_R) == BST_CHECKED)
			{
				picker->SetRGB(red, fx, fy);

				UpdateValues(hDlg, picker->GetCurrentColour());
			}

			else if (IsDlgButtonChecked(hDlg, IDC_G) == BST_CHECKED)
			{
				picker->SetRGB(fx, green, fy);

				UpdateValues(hDlg, picker->GetCurrentColour());
			}

			else if (IsDlgButtonChecked(hDlg, IDC_B) == BST_CHECKED)
			{
				picker->SetRGB(fy, fx, blue);

				UpdateValues(hDlg, picker->GetCurrentColour());
			}

			else if (IsDlgButtonChecked(hDlg, IDC_H) == BST_CHECKED)
			{
				picker->SetHSV(hue, fx / 255.0 * 100.0, (255 - fy) / 255.0 * 100.0);

				UpdateValues(hDlg, picker->GetCurrentColour());
			}

			else if (IsDlgButtonChecked(hDlg, IDC_S) == BST_CHECKED)
			{
				picker->SetHSV(fx / 255.0 * 359.0, saturation, (255 - fy) / 255.0 * 100.0);

				UpdateValues(hDlg, picker->GetCurrentColour());
			}

			else if (IsDlgButtonChecked(hDlg, IDC_V) == BST_CHECKED)
			{
				picker->SetHSV(fx / 255.0 * 359.0, (255 - fy) / 255.0 * 100.0, value);

				UpdateValues(hDlg, picker->GetCurrentColour());
			}
		}
		// IDC_COLOR2 picked
		else if (_IS_IN(rectC2.left, rectC2.right, p.x) && _IS_IN(rectC2.top, rectC2.bottom, p.y))
		{
			const float fy = ((p.y - rectC2.top) / heightC2) * 255.0f;

			if (IsDlgButtonChecked(hDlg, IDC_R) == BST_CHECKED)
			{
				picker->SetRGB(255 - fy, green, blue);

				UpdateValues(hDlg, picker->GetCurrentColour());
			}

			else if (IsDlgButtonChecked(hDlg, IDC_G) == BST_CHECKED)
			{
				picker->SetRGB(red, 255 - fy, blue);

				UpdateValues(hDlg, picker->GetCurrentColour());
			}

			else if (IsDlgButtonChecked(hDlg, IDC_B) == BST_CHECKED)
			{
				picker->SetRGB(red, green, 255 - fy);

				UpdateValues(hDlg, picker->GetCurrentColour());
			}

			else if (IsDlgButtonChecked(hDlg, IDC_H) == BST_CHECKED)
			{
				picker->SetHSV((255 - fy) / 255.0 * 359.0, saturation, value);

				UpdateValues(hDlg, picker->GetCurrentColour());
			}

			else if (IsDlgButtonChecked(hDlg, IDC_S) == BST_CHECKED)
			{
				picker->SetHSV(hue, (255 - fy) / 255.0 * 100.0, value);

				UpdateValues(hDlg, picker->GetCurrentColour());
			}

			else if (IsDlgButtonChecked(hDlg, IDC_V) == BST_CHECKED)
			{
				picker->SetHSV(hue, saturation, (255 - fy) / 255.0 * 100.0);

				UpdateValues(hDlg, picker->GetCurrentColour());
			}
		}
		// IDC_ALPHASLIDE picked
		else if (_IS_IN(rectA.left, rectA.right, p.x) && _IS_IN(rectA.top, rectA.bottom, p.y))
		{
			const float fy = ((p.y - rectA.top) / heightA) * 255.0f;

			picker->SetAlpha((255 - fy) / 255.0f * 100.0f);

			UpdateValues(hDlg, picker->GetCurrentColour());
		}

		SendMessage(hDlg, WM_PAINT, 0, 0);
		break;
	}

	case WM_COMMAND:
		switch (HIWORD(wParam))
		{
		case EN_SETFOCUS:
		{
			SendDlgItemMessage(hDlg, LOWORD(wParam), EM_SETSEL, 0, -1);
			break;
		}

		case EN_CHANGE:
		{
			if (!open || programmaticallyChangingText)
			{
				break;
			}

			programmaticallyChangingText = true;

			DWORD start;
			DWORD end;
			SendDlgItemMessage(hDlg, LOWORD(wParam), EM_GETSEL, (WPARAM)&start, (LPARAM)&end);

			switch (LOWORD(wParam))
			{
				int tempcolor;

			case IDC_RED:
			case IDC_GREEN:
			case IDC_BLUE:
			{
				tempcolor = GetDlgItemInt(hDlg, LOWORD(wParam), NULL, false);
				tempcolor = min(255, tempcolor);
				tempcolor = max(0, tempcolor);
				SetDlgItemInt(hDlg, LOWORD(wParam), tempcolor, false);

				picker->SetRGB(GetDlgItemInt(hDlg, IDC_RED, NULL, false), GetDlgItemInt(hDlg, IDC_GREEN, NULL, false), GetDlgItemInt(hDlg, IDC_BLUE, NULL, false));
				break;
			}

			case IDC_HUE:
			{
				tempcolor = GetDlgItemInt(hDlg, IDC_HUE, NULL, false);
				tempcolor = min(359, tempcolor);
				tempcolor = max(0, tempcolor);
				SetDlgItemInt(hDlg, IDC_HUE, tempcolor, false);

				picker->SetHSV(tempcolor, saturation, value);
				break;
			}

			case IDC_SATURATION:
			case IDC_VALUE:
			{
				tempcolor = GetDlgItemInt(hDlg, LOWORD(wParam), NULL, false);
				tempcolor = min(100, tempcolor);
				tempcolor = max(0, tempcolor);
				SetDlgItemInt(hDlg, LOWORD(wParam), tempcolor, false);

				picker->SetHSV(hue, GetDlgItemInt(hDlg, IDC_SATURATION, NULL, false), GetDlgItemInt(hDlg, IDC_VALUE, NULL, false));
				break;
			}

			case IDC_ALPHA:
			{
				tempcolor = GetDlgItemInt(hDlg, LOWORD(wParam), NULL, false);
				tempcolor = min(100, tempcolor);
				tempcolor = max(0, tempcolor);
				SetDlgItemInt(hDlg, LOWORD(wParam), tempcolor, false);

				picker->SetAlpha(tempcolor);
			}
			}

			// Update color
			UpdateValues(hDlg, picker->GetCurrentColour());
			programmaticallyChangingText = false;
			SendDlgItemMessage(hDlg, LOWORD(wParam), EM_SETSEL, start, end);
			SendMessage(hDlg, WM_PAINT, 0, 0);

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
				SendMessage(hDlg, WM_PAINT, 0, 0);
				break;
			}

			case IDC_OLDCOLOR:
			{
				picker->Revert();
				UpdateValues(hDlg, picker->GetCurrentColour());
				SendMessage(hDlg, WM_PAINT, 0, 0);
				break;
			}

			case IDB_OK:
			{
				open = false;
				picker->UpdateOldColour();
				pbufferC1.Destroy();
				pbufferC2.Destroy();
				pbufferA.Destroy();
				EndDialog(hDlg, IDB_OK);
				break;
			}

			case IDB_CANCEL:
			{
				open = false;
				SColour old = picker->GetOldColour();

				picker->SetRGB(old.r, old.g, old.b);
				picker->SetAlpha(old.a);
				pbufferC1.Destroy();
				pbufferC2.Destroy();
				pbufferA.Destroy();
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

// Draw a b/w checked rectangle, "covered" with the rgba color provided.
// cx and cy are the size of the checks
void DrawCheckedRect(HWND hWnd, int r, int g, int b, int a, int cx, int cy)
{
	float rf = (float)r / 255.0f,
		gf = (float)g / 255.0f,
		bf = (float)b / 255.0f,
		af = (float)a / 100.0f;
	HDC hdc = GetDC(hWnd);
	HBRUSH brush, brush2;
	RECT rect, r2;
	bool flag;

	brush = CreateSolidBrush(RGB((rf*af) * 255, (gf*af) * 255, (bf*af) * 255));
	brush2 = CreateSolidBrush(RGB((rf*af + 1 - af) * 255, (gf*af + 1 - af) * 255, (bf*af + 1 - af) * 255));

	GetWindowRect(hWnd, &rect);

	for (int x = 0; (x*cx) < WIDTH(rect); x++)
	{
		if (x % 2)
			flag = false;
		else
			flag = true;
		for (int y = 0; (y*cy) < HEIGHT(rect); y++)
		{
			r2.left = x * cx;
			r2.right = min((x + 1)*cx, WIDTH(rect) - 2);
			r2.top = y * cy;
			r2.bottom = min((y + 1)*cy, HEIGHT(rect) - 2);

			FillRect(hdc, &r2, flag ? brush : brush2);

			flag = !flag;
		}
	}

	DeleteObject(brush);
	DeleteObject(brush2);
	ReleaseDC(hWnd, hdc);
}

void DrawCircle(HDC hcomp, int red, int green, int blue, float x, float y)
{
	HPEN pen = CreatePen(PS_SOLID, 1, RGB(255 - red, 255 - green, 255 - blue));
	SelectObject(hcomp, pen);
	Arc(hcomp, x - 5, y - 5, x + 5, y + 5, 0, 0, 0, 0);
}

void DrawArrows(HDC hcomp, int width, int height, float y)
{
	HPEN pen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
	SelectObject(hcomp, pen);
	MoveToEx(hcomp, 0, height - (y - 5), NULL);
	LineTo(hcomp, 0, height - (y + 5));
	LineTo(hcomp, 5, height - (y));
	LineTo(hcomp, 0, height - (y - 5));
	MoveToEx(hcomp, width - 1, height - (y - 5), NULL);
	LineTo(hcomp, width - 1, height - (y + 5));
	LineTo(hcomp, width - 6, height - (y));
	LineTo(hcomp, width - 1, height - (y - 5));
}

void UpdateValues(HWND hDlg, SColour col)
{
	programmaticallyChangingText = true;
	TCHAR buff[10];

	SetDlgItemInt(hDlg, IDC_RED, col.r, false);
	SetDlgItemInt(hDlg, IDC_GREEN, col.g, false);
	SetDlgItemInt(hDlg, IDC_BLUE, col.b, false);
	SetDlgItemInt(hDlg, IDC_ALPHA, col.a, false);
	SetDlgItemInt(hDlg, IDC_HUE, col.h, false);
	SetDlgItemInt(hDlg, IDC_SATURATION, col.s, false);
	SetDlgItemInt(hDlg, IDC_VALUE, col.v, false);

	swprintf_s(buff, L"%02X%02X%02X", col.r, col.g, col.b);
	SetDlgItemText(hDlg, IDC_HEXCOL, buff);
	programmaticallyChangingText = false;
}