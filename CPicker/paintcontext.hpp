#pragma once
#include "../TranslucentTB/arch.h"
#include <windef.h>
#include <WinUser.h>

class PaintContext {
private:
	HWND m_handle;
	PAINTSTRUCT m_ps;

public:
	inline PaintContext(HWND hWnd) : m_handle(hWnd)
	{
		BeginPaint(m_handle, &m_ps);
	}

	inline ~PaintContext()
	{
		EndPaint(m_handle, &m_ps);
	}

	inline PaintContext(const PaintContext &) = delete;
	inline PaintContext &operator =(const PaintContext &) = delete;
};