#include "resource.h"
#include "CPickerDll.h"
#include "stdio.h"
#include <cmath>

static PixelBuffer pbuffer;

LRESULT CALLBACK ColourPickerDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
	static CColourPicker *picker = NULL;

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

	switch (uMsg)
		{
		case WM_INITDIALOG:
			{
			picker = (CColourPicker*)lParam;
			pbuffer.Create(widthC1, heightC1);

			switch (picker->GetAlphaUsage())
				{
				case CP_NO_ALPHA:
					{
					picker->SetAlpha(100);
					}
				break;

				case CP_DISABLE_ALPHA:
					{
					picker->SetAlpha(100);
					EnableWindow(GetDlgItem(hDlg, IDC_ALPHA), false);
					EnableWindow(GetDlgItem(hDlg, IDC_ALPHATXT), false);
					EnableWindow(GetDlgItem(hDlg, IDC_ALPHATXT2), false);
					}
				break;
				}

			UpdateValues(hDlg, picker->GetCurrentColour());

			SendDlgItemMessage(hDlg, IDC_R, BM_SETCHECK, BST_CHECKED, 0);
			}
		break;

		case WM_PAINT:
			{
			HDC hdc, hcomp;
			HBITMAP hbmp;
			RECT rect;
			HPEN pen;
			HBRUSH brush;
			int red, green, blue, alpha;
			float rf, gf, bf;

            // Big color selector (displays non-selected features)
			// For example, if G is selected (in RGB radio box) it displays red vs. blue colors
			hdc = GetDC(Color1);
			hcomp = CreateCompatibleDC(hdc);

			hbmp = CreateCompatibleBitmap(hdc, widthC1, heightC1);
			SelectObject(hcomp, hbmp);
			
			red = GetDlgItemInt(hDlg, IDC_RED, NULL, false);
			green = GetDlgItemInt(hDlg, IDC_GREEN, NULL, false);
			blue = GetDlgItemInt(hDlg, IDC_BLUE, NULL, false);
			alpha = GetDlgItemInt(hDlg, IDC_ALPHA, NULL, false);

			// Check who is selected.
			// RED
			if (IsDlgButtonChecked(hDlg, IDC_R) == BST_CHECKED)
				{
				for (int g=0; g < widthC1; g++)
					for (int b=0; b < heightC1; b++)
					{
						const float fgreen = std::round((g / widthC1) * 255.0f);
						const float fblue = std::round((b / heightC1) * 255.0f);
						pbuffer.SetPixel(g, b, RGB(red, fgreen, fblue));
					}

				pbuffer.Display(hcomp);

				
				// Draws circle
				pen = CreatePen(PS_SOLID, 1, RGB(255-red, 255-green, 255-blue));
				SelectObject(hcomp, pen);
				const float fgreen = std::round((green / 255.0f) * widthC1);
				const float fblue = std::round((blue / 255.0f) * heightC1);
				Arc(hcomp, fgreen-5, fblue-5, fgreen+5, fblue+5, 0, 0, 0, 0);
				}
			// GREEN
			else if (IsDlgButtonChecked(hDlg, IDC_G) == BST_CHECKED)
				{
				for (int r=0; r<widthC1; r++)
					for (int b = 0; b < heightC1; b++)
					{
						const float fred = std::round((r / widthC1) * 255.0f);
						const float fblue = std::round((b / heightC1) * 255.0f);
						pbuffer.SetPixel(r, b, RGB(fred, green, fblue));
					}
				pbuffer.Display(hcomp);

				// Draws circle
				pen = CreatePen(PS_SOLID, 1, RGB(255-red, 255-green, 255-blue));
				SelectObject(hcomp, pen);
				const float fred = std::round((red / 255.0f) * widthC1);
				const float fblue = std::round((blue / 255.0f) * heightC1);
				Arc(hcomp, fred-5, fblue-5, fred+5, fblue+5, 0, 0, 0, 0);
				}
			// BLUE
			else if (IsDlgButtonChecked(hDlg, IDC_B) == BST_CHECKED)
				{
				for (int g=0; g<widthC1; g++)
					for (int r = 0; r < heightC1; r++)
					{
						const float fred = std::round((r / heightC1) * 255.0f);
						const float fgreen = std::round((g / widthC1) * 255.0f);
						pbuffer.SetPixel(g, r, RGB(fred, fgreen, blue));
					}
				pbuffer.Display(hcomp);
                
				// Draws circle
				pen = CreatePen(PS_SOLID, 1, RGB(255-red, 255-green, 255-blue));
				SelectObject(hcomp, pen);
				Arc(hcomp, green-5, red-5, green+5, red+5, 0, 0, 0, 0);
				}

			// HUE
			else if (IsDlgButtonChecked(hDlg, IDC_H) == BST_CHECKED)
				{
				double sat, val, stepsat, stepval;
				SColour tempcol;

				sat, val = 0.0;
				stepsat = 100.0 / widthC1;
				stepval = 100.0 / heightC1;
				
				tempcol.h = (unsigned short) GetDlgItemInt(hDlg, IDC_HUE, NULL, false);
				tempcol.s = (unsigned short) sat;
				tempcol.v = (unsigned short) val;

				for (int y = heightC1 - 1; y > -1; y--)
				{
					for (int x = 0; x < widthC1; x++)
					{
						sat += stepsat;
						tempcol.s = (unsigned short)sat;
						tempcol.UpdateRGB();
						pbuffer.SetPixel(x, y, RGB(tempcol.r, tempcol.g, tempcol.b));
					}
					
					val += stepval;
					sat = 0.0;
					tempcol.v = (unsigned short) val;
				}
				pbuffer.Display(hcomp);
                
				// Draws circle
				tempcol.s = (unsigned short) GetDlgItemInt(hDlg, IDC_SATURATION, NULL, false);
				tempcol.v = (unsigned short) GetDlgItemInt(hDlg, IDC_VALUE, NULL, false);
				tempcol.UpdateRGB();

				pen = CreatePen(PS_SOLID, 1, RGB(255-tempcol.r, 255-tempcol.g, 255-tempcol.b));
				SelectObject(hcomp, pen);
				
				Arc(hcomp, (int)(tempcol.s/stepsat)-5, heightC1-(int)(tempcol.v / stepval) + 5, (int)(tempcol.s/stepsat)+5,
					heightC1-(int)(tempcol.v / stepval) - 5, 0, 0, 0, 0);
				}
            
			// SATURATION
			else if (IsDlgButtonChecked(hDlg, IDC_S) == BST_CHECKED)
				{
				double hue, val, stepval, stephue;
				SColour tempcol;

				hue, val = 0.0;
				stephue = 359.0/widthC1;
				stepval = 100.0/heightC1;
				
				tempcol.h = (unsigned short) hue;
				tempcol.s = (unsigned short) GetDlgItemInt(hDlg, IDC_SATURATION, NULL, false);
				tempcol.v = (unsigned short) val;

				for (int y = heightC1 - 1; y > -1; y--)
				{
					for (int x = 0; x < widthC1; x++)
					{
						hue += stephue;
						tempcol.h = (unsigned short)hue;
						tempcol.UpdateRGB();
						pbuffer.SetPixel(x, y, RGB(tempcol.r, tempcol.g, tempcol.b));
					}

					val += stepval;
					hue = 0.0;
					tempcol.v = (unsigned short) val;
				}
				pbuffer.Display(hcomp);
                
				// Draws circle
				tempcol.h = (unsigned short) GetDlgItemInt(hDlg, IDC_HUE, NULL, false);
				tempcol.v = (unsigned short) GetDlgItemInt(hDlg, IDC_VALUE, NULL, false);
				tempcol.UpdateRGB();

				pen = CreatePen(PS_SOLID, 1, RGB(255-tempcol.r, 255-tempcol.g, 255-tempcol.b));
				SelectObject(hcomp, pen);
				
				Arc(hcomp, (int)(tempcol.h/stephue)-5, heightC1-(int)(tempcol.v/stepval)+5, (int)(tempcol.h/stephue)+5,
					heightC1-(int)(tempcol.v/stepval)-5, 0, 0, 0, 0);
				}
            
			// VALUE
			else if (IsDlgButtonChecked(hDlg, IDC_V) == BST_CHECKED)
				{
				double hue, sat, stepsat, stephue;
				SColour tempcol;

				hue, sat = 0.0;
				stephue = 359.0/widthC1;
				stepsat = 100.0/heightC1;

				tempcol.h = (unsigned short) hue;
				tempcol.s = (unsigned short) sat;
				tempcol.v = (unsigned short) GetDlgItemInt(hDlg, IDC_VALUE, NULL, false);

				for (int y = heightC1 - 1; y > -1; y--)
				{
					for (int x = 0; x < widthC1; x++)
					{
						hue += stephue;
						tempcol.h = (unsigned short)hue;
						tempcol.UpdateRGB();
						pbuffer.SetPixel(x, y, RGB(tempcol.r, tempcol.g, tempcol.b));
					}

					sat += stepsat;
					hue = 0.0;
					tempcol.s = (unsigned short) sat;
				}
				pbuffer.Display(hcomp);
                
				// Draws circle
				tempcol.h = GetDlgItemInt(hDlg, IDC_HUE, NULL, false);
				tempcol.s = GetDlgItemInt(hDlg, IDC_SATURATION, NULL, false);
				tempcol.UpdateRGB();

				pen = CreatePen(PS_SOLID, 1, RGB(255-tempcol.r, 255-tempcol.g, 255-tempcol.b));
				SelectObject(hcomp, pen);
				
				Arc(hcomp, (int)(tempcol.h/stephue)-5, heightC1-(int)(tempcol.s/stepsat)+5, (int)(tempcol.h/stephue)+5, 
					heightC1-(int)(tempcol.s/stepsat)-5, 0, 0, 0, 0);
				}

			BitBlt(hdc, 0, 0, rectC1.right, rectC1.bottom, hcomp, 0, 0, SRCCOPY);

			DeleteObject(hbmp);
			DeleteDC(hcomp);
			ReleaseDC(Color1, hdc);

			// Small color selector (displays selected feature)
			hdc = GetDC(Color2);
			hcomp = CreateCompatibleDC(hdc);

			GetClientRect(Color2, &rect);
			hbmp = CreateCompatibleBitmap(hdc, rect.right, 255);
			SelectObject(hcomp, hbmp);

			brush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
			FillRect(hcomp, &rect, brush);
			DeleteObject(brush);
			
			// Check who is selected.
			// RED
			if (IsDlgButtonChecked(hDlg, IDC_R) == BST_CHECKED)
				{
				for (int r=255; r>0; r--)
					{
					int g = GetDlgItemInt(hDlg, IDC_GREEN, NULL, false);
					int b = GetDlgItemInt(hDlg, IDC_BLUE, NULL, false);
					
					for (int x=6; x<rect.right-6; x++)
						SetPixel(hcomp, x, r, RGB(255-r, g, b));
					}

				// Draws arrows
				pen = CreatePen(PS_SOLID, 1, RGB(0,0,0));
				SelectObject(hcomp, pen);
				MoveToEx(hcomp, 0, 255-(red-5), NULL);
                LineTo(hcomp, 0, 255-(red+5));
				LineTo(hcomp, 5, 255-(red));
				LineTo(hcomp, 0, 255-(red-5));
				MoveToEx(hcomp, rect.right-1, 255-(red-5), NULL);
				LineTo(hcomp, rect.right-1, 255-(red+5));
				LineTo(hcomp, rect.right-6, 255-(red));
				LineTo(hcomp, rect.right-1, 255-(red-5));
				}
			// GREEN
			else if (IsDlgButtonChecked(hDlg, IDC_G) == BST_CHECKED)
				{
				for (int g=255; g>0; g--)
					{
					int r = GetDlgItemInt(hDlg, IDC_RED, NULL, false);
					int b = GetDlgItemInt(hDlg, IDC_BLUE, NULL, false);

					for (int x=6; x<rect.right-6; x++)
						SetPixel(hcomp, x, g, RGB(r, 255-g, b));
					}

				pen = CreatePen(PS_SOLID, 1, RGB(0,0,0));
				SelectObject(hcomp, pen);
				MoveToEx(hcomp, 0, 255-(green-5), NULL);
                LineTo(hcomp, 0, 255-(green+5));
				LineTo(hcomp, 5, 255-(green));
				LineTo(hcomp, 0, 255-(green-5));
				MoveToEx(hcomp, rect.right-1, 255-(green-5), NULL);
				LineTo(hcomp, rect.right-1, 255-(green+5));
				LineTo(hcomp, rect.right-6, 255-(green));
				LineTo(hcomp, rect.right-1, 255-(green-5));
				}
            // BLUE
			else if (IsDlgButtonChecked(hDlg, IDC_B) == BST_CHECKED)
				{
				for (int b=255; b>0; b--)
					{
					int g = GetDlgItemInt(hDlg, IDC_GREEN, NULL, false);
					int r = GetDlgItemInt(hDlg, IDC_RED, NULL, false);

					for (int x=6; x<rect.right-6; x++)
						SetPixel(hcomp, x, b, RGB(r, g, 255-b));
					}

				pen = CreatePen(PS_SOLID, 1, RGB(0,0,0));
				SelectObject(hcomp, pen);
				MoveToEx(hcomp, 0, 255-(blue-5), NULL);
                LineTo(hcomp, 0, 255-(blue+5));
				LineTo(hcomp, 5, 255-(blue));
				LineTo(hcomp, 0, 255-(blue-5));
				MoveToEx(hcomp, rect.right-1, 255-(blue-5), NULL);
				LineTo(hcomp, rect.right-1, 255-(blue+5));
				LineTo(hcomp, rect.right-6, 255-(blue));
				LineTo(hcomp, rect.right-1, 255-(blue-5));
				}
			// HUE
			else if (IsDlgButtonChecked(hDlg, IDC_H) == BST_CHECKED)
				{
				double hue, step;
				SColour tempcol;

				hue = 0.0;

				tempcol.h = 0;
				tempcol.s = 100;
				tempcol.v = 100;

				step = 359.0/255.0;

				for (int y=255; y>0; y--) // NB: 255 is the height of IDC_COLOR and IDC_COLOR1
					{
					tempcol.UpdateRGB();

					for (int x=6; x<rect.right-6; x++)
						SetPixel(hcomp, x, y, RGB(tempcol.r, tempcol.g, tempcol.b));

					hue += step;
					tempcol.h = (UINT) hue;
					}

				hue = (double) GetDlgItemInt(hDlg, IDC_HUE, NULL, false);
				hue = hue / step;

				// Draws arrows
				pen = CreatePen(PS_SOLID, 1, RGB(0,0,0));
				SelectObject(hcomp, pen);
				MoveToEx(hcomp, 0, 255 - ((int)(hue)-5), NULL);
                LineTo(hcomp, 0, 255 - ((int)(hue)+5));
				LineTo(hcomp, 5, 255 - ((int)(hue)));
				LineTo(hcomp, 0, 255 - ((int)(hue)-5));
				MoveToEx(hcomp, rect.right-1, 255 - ((int)(hue)-5), NULL);
				LineTo(hcomp, rect.right-1, 255 - ((int)(hue)+5));
				LineTo(hcomp, rect.right-6, 255 - ((int)(hue)));
				LineTo(hcomp, rect.right-1, 255 - ((int)(hue)-5));
				}
            // SATURATION
			else if (IsDlgButtonChecked(hDlg, IDC_S) == BST_CHECKED)
				{
				double sat, step;
				SColour tempcol;

				sat = 0.0;

				tempcol.h = (unsigned short) GetDlgItemInt(hDlg, IDC_HUE, NULL, false);
				tempcol.s = 0;
				tempcol.v = (unsigned short) GetDlgItemInt(hDlg, IDC_VALUE, NULL, false);
                                    
				step = 100.0/255.0;

				for (int y=255; y>0; y--) // NB: 255 is the height of IDC_COLOR and IDC_COLOR1
					{
					tempcol.UpdateRGB();
					
					for (int x=6; x<rect.right-6; x++)
						SetPixel(hcomp, x, y, RGB(tempcol.r, tempcol.g, tempcol.b));

					sat += step;
					tempcol.s = (UINT) sat;
					}

				sat = (double) GetDlgItemInt(hDlg, IDC_SATURATION, NULL, false);
				sat = sat / step;

				// Draws arrows
				pen = CreatePen(PS_SOLID, 1, RGB(0,0,0));
				SelectObject(hcomp, pen);
				MoveToEx(hcomp, 0, 255 - ((int)(sat)-5), NULL);
                LineTo(hcomp, 0, 255 - ((int)(sat)+5));
				LineTo(hcomp, 5, 255 - ((int)(sat)));
				LineTo(hcomp, 0, 255 - ((int)(sat)-5));
				MoveToEx(hcomp, rect.right-1, 255 - ((int)(sat)-5), NULL);
				LineTo(hcomp, rect.right-1, 255 - ((int)(sat)+5));
				LineTo(hcomp, rect.right-6, 255 - ((int)(sat)));
				LineTo(hcomp, rect.right-1, 255 - ((int)(sat)-5));
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
                                    
				step = 100.0/255.0;

				for (int y=255; y>0; y--) // NB: 255 is the height of IDC_COLOR and IDC_COLOR1
					{
					tempcol.UpdateRGB();
					
					for (int x=6; x<rect.right-6; x++)
						SetPixel(hcomp, x, y, RGB(tempcol.r, tempcol.g, tempcol.b));

					val += step;
					tempcol.v = (unsigned short) val;
					}

				val = (double) GetDlgItemInt(hDlg, IDC_VALUE, NULL, false);
				val  = val / step;

				// Draws arrows
				pen = CreatePen(PS_SOLID, 1, RGB(0,0,0));
				SelectObject(hcomp, pen);
				MoveToEx(hcomp, 0, 255 - ((int)(val)-5), NULL);
                LineTo(hcomp, 0, 255 - ((int)(val)+5));
				LineTo(hcomp, 5, 255 - ((int)(val)));
				LineTo(hcomp, 0, 255 - ((int)(val)-5));
				MoveToEx(hcomp, rect.right-1, 255 - ((int)(val)-5), NULL);
				LineTo(hcomp, rect.right-1, 255 - ((int)(val)+5));
				LineTo(hcomp, rect.right-6, 255 - ((int)(val)));
				LineTo(hcomp, rect.right-1, 255 - ((int)(val)-5));
				}
			
			BitBlt(hdc, 0, 0, rect.right, rect.bottom, hcomp, 0, 0, SRCCOPY);

			DeleteObject(hbmp);
			DeleteDC(hcomp);
			ReleaseDC(Color2, hdc);
  
			// Alpha slider
			if (picker->GetAlphaUsage() != CP_NO_ALPHA)
				{
				hdc = GetDC(Alpha);
				hcomp = CreateCompatibleDC(hdc);

				GetClientRect(Alpha, &rect);
				hbmp = CreateCompatibleBitmap(hdc, rect.right, 255);
				SelectObject(hcomp, hbmp);

				brush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
				FillRect(hcomp, &rect, brush);
				DeleteObject(brush);

				rf = (float)red/255.0f;
				gf = (float)green/255.0f;
				bf = (float)blue/255.0f;
				bool flag = false;

				for (int y=255; y>0; y--)
					{
					COLORREF cb, cw;

					if (!(y%(WIDTH(rect)/2-6)))
						flag = !flag;

					float af = 1.0f - (float)y/255.0f;

					cb = RGB((rf*af)*255, (gf*af)*255, (bf*af)*255);
					cw = RGB((rf*af+1-af)*255, (gf*af+1-af)*255, (bf*af+1-af)*255);

					if (flag)
						{
						for (int x=6; x<(WIDTH(rect)/2); x++)
							SetPixel(hcomp, x, y, cw);
						for (int x=(WIDTH(rect)/2); x<rect.right-6; x++)
							SetPixel(hcomp, x, y, cb);
						}
					else
						{
						for (int x=6; x<(WIDTH(rect)/2); x++)
							SetPixel(hcomp, x, y, cb);
						for (int x=(WIDTH(rect)/2); x<rect.right-6; x++)
							SetPixel(hcomp, x, y, cw);
						}
					}

				if (picker->GetAlphaUsage() == CP_USE_ALPHA)
					{
					// Draws arrows
					int a;

					a = (int)((float)alpha/100.0f*255.0f);
					pen = CreatePen(PS_SOLID, 1, RGB(0,0,0));
					SelectObject(hcomp, pen);
					MoveToEx(hcomp, 0, 255-(a-5), NULL);
					LineTo(hcomp, 0, 255-(a+5));
					LineTo(hcomp, 5, 255-(a));
					LineTo(hcomp, 0, 255-(a-5));
					MoveToEx(hcomp, rect.right-1, 255-(a-5), NULL);
					LineTo(hcomp, rect.right-1, 255-(a+5));
					LineTo(hcomp, rect.right-6, 255-(a));
					LineTo(hcomp, rect.right-1, 255-(a-5));
					}

				BitBlt(hdc, 0, 0, rect.right, rect.bottom, hcomp, 0, 0, SRCCOPY);

				DeleteObject(hbmp);
				DeleteDC(hcomp);
				ReleaseDC(Alpha, hdc);
				}

			// Current color & old color
			HWND Color = GetDlgItem(hDlg, IDC_CURRCOLOR);

			DrawCheckedRect(Color, picker->GetCurrentColour().r, picker->GetCurrentColour().g, 
				picker->GetCurrentColour().b, picker->GetCurrentColour().a, 10, 10);
			
			Color = GetDlgItem(hDlg, IDC_OLDCOLOR);
			
			DrawCheckedRect(Color, picker->GetOldColour().r, picker->GetOldColour().g, picker->GetOldColour().b, 
				picker->GetOldColour().a, 10, 10);
			}
		break; // WM_PAINT

		case WM_LBUTTONDOWN:
		case WM_MOUSEMOVE:
			{
			POINT p;

			if (uMsg == WM_MOUSEMOVE && wParam != MK_LBUTTON)
				break;

			GetCursorPos(&p);
			
			// IDC_COLOR1 picked
			if (_IS_IN(rectC1.left, rectC1.right, p.x) && 
				_IS_IN(rectC1.top, rectC1.bottom, p.y))
				{
					const float fx = std::round(((p.x - rectC1.left) / widthC1) * 255);
					const float fy = std::round(((p.y - rectC1.top) / heightC1) * 255);

				if (IsDlgButtonChecked(hDlg, IDC_R) == BST_CHECKED)
				{
					picker->SetRGB((unsigned short)GetDlgItemInt(hDlg, IDC_RED, NULL, false), fx, fy);
					
					UpdateValues(hDlg, picker->GetCurrentColour());
				}

				else if (IsDlgButtonChecked(hDlg, IDC_G) == BST_CHECKED)
					{
					picker->SetRGB(fx, (unsigned short)GetDlgItemInt(hDlg, IDC_GREEN, NULL, false), fy);

					UpdateValues(hDlg, picker->GetCurrentColour());
					}

				else if (IsDlgButtonChecked(hDlg, IDC_B) == BST_CHECKED)
					{
					picker->SetRGB(fy, fx, (unsigned short) GetDlgItemInt(hDlg, IDC_BLUE, NULL, false));

					UpdateValues(hDlg, picker->GetCurrentColour());
					} 

				else if (IsDlgButtonChecked(hDlg, IDC_H) == BST_CHECKED)
					{
					picker->SetHSV((unsigned short)GetDlgItemInt(hDlg, IDC_HUE, NULL, false),
						(unsigned short)(fx/255.0*100.0), 
						(unsigned short)((255-fy)/255.0*100.0));

					UpdateValues(hDlg, picker->GetCurrentColour());					
					}

				else if (IsDlgButtonChecked(hDlg, IDC_S) == BST_CHECKED)
					{
					picker->SetHSV((unsigned short)(fx/255.0*359.0),
						(unsigned short)GetDlgItemInt(hDlg, IDC_SATURATION, NULL, false), 
						(unsigned short)((255-fy)/255.0*100.0));

					UpdateValues(hDlg, picker->GetCurrentColour());					
					} 
				
				else if (IsDlgButtonChecked(hDlg, IDC_V) == BST_CHECKED)
					{
					picker->SetHSV((unsigned short)(fx/255.0*359.0),
						(unsigned short)((255-fy)/255.0*100.0), 
						(unsigned short)GetDlgItemInt(hDlg, IDC_VALUE, NULL, false));

					UpdateValues(hDlg, picker->GetCurrentColour());					
					}
				}
			// IDC_COLOR2 picked
			else if (_IS_IN(rectC2.left-10, rectC2.right+10, p.x) && 
				_IS_IN(rectC2.top-10, rectC2.bottom+10, p.y))
				{
					int y = (p.y-rectC2.top);
					CLAMP(y,0,255);

				if (IsDlgButtonChecked(hDlg, IDC_R) == BST_CHECKED)
					{
					picker->SetRGB((unsigned short)(255 - y),
						(unsigned short) GetDlgItemInt(hDlg, IDC_GREEN, NULL, false),
						(unsigned short) GetDlgItemInt(hDlg, IDC_BLUE, NULL, false));

					UpdateValues(hDlg, picker->GetCurrentColour());
					}

				else if (IsDlgButtonChecked(hDlg, IDC_G) == BST_CHECKED)
					{
					picker->SetRGB((unsigned short) GetDlgItemInt(hDlg, IDC_RED, NULL, false),
						(unsigned short) (255 - y),
						(unsigned short) GetDlgItemInt(hDlg, IDC_BLUE, NULL, false));

					UpdateValues(hDlg, picker->GetCurrentColour());
					}
				
				else if (IsDlgButtonChecked(hDlg, IDC_B) == BST_CHECKED)
					{
					picker->SetRGB((unsigned short) GetDlgItemInt(hDlg, IDC_RED, NULL, false),
						(unsigned short) GetDlgItemInt(hDlg, IDC_GREEN, NULL, false),
						(unsigned short) (255 - y));

					UpdateValues(hDlg, picker->GetCurrentColour());
					}

				else if (IsDlgButtonChecked(hDlg, IDC_H) == BST_CHECKED)
					{
					picker->SetHSV((unsigned short) ((255 - y)/255.0*359.0),
						(unsigned short) GetDlgItemInt(hDlg, IDC_SATURATION, NULL, false),
						(unsigned short) GetDlgItemInt(hDlg, IDC_VALUE, NULL, false));

					UpdateValues(hDlg, picker->GetCurrentColour());
					}

				else if (IsDlgButtonChecked(hDlg, IDC_S) == BST_CHECKED)
					{
					picker->SetHSV((unsigned short) GetDlgItemInt(hDlg, IDC_HUE, NULL, false),
						(unsigned short) ((255 - y)/255.0*100.0),
						(unsigned short) GetDlgItemInt(hDlg, IDC_VALUE, NULL, false));

					UpdateValues(hDlg, picker->GetCurrentColour());
					}

				else if (IsDlgButtonChecked(hDlg, IDC_V) == BST_CHECKED)
					{
					picker->SetHSV((unsigned short) GetDlgItemInt(hDlg, IDC_HUE, NULL, false),
						(unsigned short) GetDlgItemInt(hDlg, IDC_SATURATION, NULL, false),
						(unsigned short) ((255 - y)/255.0*100.0));

					UpdateValues(hDlg, picker->GetCurrentColour());
					}
				}
			// IDC_ALPHASLIDE picked
			else if (_IS_IN(rectA.left-10, rectA.right+10, p.x) &&
				_IS_IN(rectA.top-10, rectA.bottom+10, p.y) && (picker->GetAlphaUsage()==CP_USE_ALPHA))
				{
				int y = (p.y-rectA.top);
				CLAMP(y,0,255);

				picker->SetAlpha((unsigned short)((float)(255 - y)/255.0f*100.0f));

				UpdateValues(hDlg, picker->GetCurrentColour());
				}

			SendMessage(hDlg, WM_PAINT, 0, 0);
			}
		break; // WM_LBUTTONDOWN, WM_MOUSEMOVE		

		case WM_COMMAND:
			switch(HIWORD(wParam))
				{
				case EN_SETFOCUS:
					{
					SendDlgItemMessage(hDlg, LOWORD(wParam), EM_SETSEL, 0, -1);
					}
				break;

				case EN_KILLFOCUS:
					{
					switch(LOWORD(wParam))
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

							picker->SetRGB(GetDlgItemInt(hDlg, IDC_RED, NULL, false),
								GetDlgItemInt(hDlg, IDC_GREEN, NULL, false), 
								GetDlgItemInt(hDlg, IDC_BLUE, NULL, false));
							}
						break;

						case IDC_HUE:
							{
							tempcolor = GetDlgItemInt(hDlg, IDC_HUE, NULL, false);
							tempcolor = min(359, tempcolor);
							tempcolor = max(0, tempcolor);
							SetDlgItemInt(hDlg, IDC_HUE, tempcolor, false);

							picker->SetHSV(GetDlgItemInt(hDlg, IDC_HUE, NULL, false),
								GetDlgItemInt(hDlg, IDC_SATURATION, NULL, false), 
								GetDlgItemInt(hDlg, IDC_VALUE, NULL, false));
							}
						break;

						case IDC_SATURATION:
						case IDC_VALUE:
							{
							tempcolor = GetDlgItemInt(hDlg, IDC_SATURATION, NULL, false);
							tempcolor = min(100, tempcolor);
							tempcolor = max(0, tempcolor);
							SetDlgItemInt(hDlg, IDC_SATURATION, tempcolor, false);							

							picker->SetHSV(GetDlgItemInt(hDlg, IDC_HUE, NULL, false),
								GetDlgItemInt(hDlg, IDC_SATURATION, NULL, false), 
								GetDlgItemInt(hDlg, IDC_VALUE, NULL, false));
							}
						break;
						} 

					// Update color
					UpdateValues(hDlg, picker->GetCurrentColour());
					SendMessage(hDlg, WM_PAINT, 0, 0);
					}
				break;
				
				case BN_CLICKED: // Equivalent to STN_CLICKED
					{
					switch(LOWORD(wParam))
						{
						case IDC_R:
						case IDC_B:
						case IDC_G:
						case IDC_H:
						case IDC_S:
						case IDC_V:
							{
							SendMessage(hDlg, WM_PAINT, 0, 0);
							}
						break;

						case IDC_OLDCOLOR:
							{
							picker->Revert();
							UpdateValues(hDlg, picker->GetCurrentColour());
							SendMessage(hDlg, WM_PAINT, 0, 0);
							}
						break;

						case IDB_OK:
							{
							picker->UpdateOldColour();
							pbuffer.Destroy();
							EndDialog(hDlg, IDB_OK);
							}
						break;
 
						case IDB_CANCEL:
							{
							SColour old = picker->GetOldColour();
 
							picker->SetRGB(old.r, old.g, old.b);
							picker->SetAlpha(old.a);
							pbuffer.Destroy();
							EndDialog(hDlg, IDB_CANCEL);
							}
						break;
						}
					}
				break;
				}
		break; // WM_COMMAND

		case WM_DPICHANGED:
			pbuffer.Destroy();
			pbuffer.Create(widthC1, heightC1);
			SendMessage(hDlg, WM_PAINT, 0, 0);
		break; // WM_DPICHANGED
		}
	return 0L;
	}

// Draw a b/w checked rectangle, "covered" with the rgba color provided.
// cx and cy are the size of the checks
EXPORT void DrawCheckedRect(HWND hWnd, int r, int g, int b, int a, int cx, int cy)
	{
	float rf = (float)r/255.0f,
		gf = (float)g/255.0f,
		bf = (float)b/255.0f,
		af = (float)a/100.0f;
	HDC hdc = GetDC(hWnd);
	HBRUSH brush, brush2;
	RECT rect, r2;
	bool flag;

	brush = CreateSolidBrush(RGB((rf*af)*255, (gf*af)*255, (bf*af)*255));
	brush2 = CreateSolidBrush(RGB((rf*af+1-af)*255, (gf*af+1-af)*255, (bf*af+1-af)*255));

	GetWindowRect(hWnd, &rect);

	for (int x=0; (x*cx)<WIDTH(rect); x++)
		{
		if (x%2)
			flag = false;
		else
			flag = true;
		for (int y=0; (y*cy)<HEIGHT(rect); y++)
			{
			r2.left = x*cx;
			r2.right = min((x+1)*cx, WIDTH(rect)-2);
			r2.top = y*cy;
			r2.bottom = min((y+1)*cy, HEIGHT(rect)-2);
			
			if (flag)
				FillRect(hdc, &r2, brush);
			else
				FillRect(hdc, &r2, brush2);

			flag = !flag;
			}
		}

	DeleteObject(brush);
	DeleteObject(brush2);
	ReleaseDC(hWnd, hdc);
	}

void UpdateValues(HWND hDlg, SColour col)
	{
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
	}