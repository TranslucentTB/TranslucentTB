#include "CPickerDll.h"

PixelBuffer::PixelBuffer()
	{
	hBmpBuffer=NULL;
	lpBits=NULL;
	}

void PixelBuffer::SetPixel(int x, int y, unsigned int color)
	{
	unsigned int *pixel = (unsigned int*)lpBits;
	pixel[(((h-1)-y)*w+x)] = color;
	}

void PixelBuffer::Display(HDC dc)
	{
	HDC hdcTemp = CreateCompatibleDC(dc);
	HGDIOBJ prev = SelectObject(hdcTemp, hBmpBuffer);
	BitBlt(dc, 0, 0, w, h, hdcTemp, 0, 0, SRCCOPY);
	SelectObject(hdcTemp, prev);
	}

void PixelBuffer::Create(int _w, int _h)
	{
	w=_w;
	h=_h;
	BITMAPV5HEADER bi;

	ZeroMemory(&bi,sizeof(BITMAPV5HEADER));
	bi.bV5Size			= sizeof(BITMAPV5HEADER);
	bi.bV5Width			= w;
	bi.bV5Height		= h;
	bi.bV5Planes		= 1;
	bi.bV5BitCount		= 32;
	bi.bV5Compression	= BI_BITFIELDS;
	// The following mask specification specifies a supported 32 BPP
	// alpha format for Windows XP.
	bi.bV5RedMask		=  0x000000FF;
	bi.bV5GreenMask		=  0x0000FF00;
	bi.bV5BlueMask		=  0x00FF0000;
	bi.bV5AlphaMask		=  0xFF000000;
	// Create the DIB section with an alpha channel.
	HDC hdc;
	hdc = GetDC(NULL);
	hBmpBuffer = CreateDIBSection(hdc, (BITMAPINFO *)&bi, DIB_RGB_COLORS,
		(void **)&lpBits, NULL, (DWORD)0);
	}

void PixelBuffer::Destroy()
	{
	if (hBmpBuffer)
		DeleteObject(hBmpBuffer);
	}
