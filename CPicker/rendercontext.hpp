#pragma once
#include <atlbase.h>
#include <d2d1_3.h>
#include <dxgi1_2.h>
#include <d3d11.h>

#include "SColour.hpp"

class RenderContext {
private:
	static CComPtr<ID2D1Factory3> m_factory;
	static HRESULT CreateDevice(const D3D_DRIVER_TYPE &type, CComPtr<ID3D11Device> &device);

	CComPtr<IDXGISwapChain1> m_swapChain;

protected:
	CComPtr<ID2D1DeviceContext2> m_dc;
	CComPtr<ID2D1SolidColorBrush> m_brush;
	D2D1_SIZE_F m_size;

	HRESULT CreateGradient(CComPtr<ID2D1LinearGradientBrush> &brush, const D2D1_COLOR_F &top, const D2D1_COLOR_F &bottom);

	virtual void ReleaseAll();

	inline HRESULT EndDraw()
	{
		HRESULT hr = m_dc->EndDraw();
		if (FAILED(hr))
		{
			return hr;
		}

		hr = m_swapChain->Present(0, 0);
		if (FAILED(hr))
		{
			return hr;
		}

		return S_OK;
	}

public:
	virtual HRESULT Refresh(HWND hwnd);
	virtual HRESULT Draw(const HWND hDlg, const SColourF &col, const SColourF &old) = 0;

	inline RenderContext() { }

	inline RenderContext(const RenderContext &) = delete;
	inline RenderContext &operator =(const RenderContext &) = delete;
};