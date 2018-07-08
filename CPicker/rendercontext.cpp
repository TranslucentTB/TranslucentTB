#include "rendercontext.hpp"
#include "dlldata.hpp"

HRESULT RenderContext::CreateDevice(const D3D_DRIVER_TYPE &type, CComPtr<ID3D11Device> &device)
{
	UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
	flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	return D3D11CreateDevice(nullptr, type, nullptr, flags, nullptr, 0, D3D11_SDK_VERSION, &device, nullptr, nullptr);
}

HRESULT RenderContext::CreateGradient(CComPtr<ID2D1LinearGradientBrush> &brush, const D2D1_COLOR_F &top, const D2D1_COLOR_F &bottom)
{
	const D2D1_GRADIENT_STOP gradientStops[] = {
		{
			0.0f,
			top
		},
		{
			1.0f,
			bottom
		}
	};

	HRESULT hr;
	CComPtr<ID2D1GradientStopCollection> pGradientStops;
	hr = m_dc->CreateGradientStopCollection(
		gradientStops,
		2,
		D2D1_GAMMA_1_0,
		D2D1_EXTEND_MODE_CLAMP,
		&pGradientStops
	);
	if (FAILED(hr))
	{
		return hr;
	}

	hr = m_dc->CreateLinearGradientBrush(
		D2D1::LinearGradientBrushProperties(
			D2D1::Point2F(0.0f, 0.0f),
			D2D1::Point2F(0.0f, m_size.height)
		),
		pGradientStops,
		&brush
	);

	if (FAILED(hr))
	{
		return hr;
	}

	return S_OK;
}

void RenderContext::ReleaseAll()
{
	m_brush.Release();
	m_dc.Release();
	m_swapChain.Release();
}

HRESULT RenderContext::Refresh(HWND hwnd)
{
	ReleaseAll();

	HRESULT hr;
	if (!m_factory)
	{
		const D2D1_FACTORY_OPTIONS fo = {
#ifdef _DEBUG
			D2D1_DEBUG_LEVEL_INFORMATION
#endif
		};

		hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, fo, &m_factory);
		if (FAILED(hr))
		{
			return hr;
		}
	}

	CComPtr<ID3D11Device> d3device;
	hr = CreateDevice(D3D_DRIVER_TYPE_HARDWARE, d3device);
	if (hr == DXGI_ERROR_UNSUPPORTED)
	{
		hr = CreateDevice(D3D_DRIVER_TYPE_WARP, d3device);
	}

	if (FAILED(hr))
	{
		return hr;
	}

	CComPtr<IDXGIDevice1> dxdevice;
	hr = d3device.QueryInterface(&dxdevice);
	if (FAILED(hr))
	{
		return hr;
	}

	hr = dxdevice->SetMaximumFrameLatency(1);
	if (FAILED(hr))
	{
		return hr;
	}

	CComPtr<IDXGIAdapter> adapter;
	hr = dxdevice->GetAdapter(&adapter);
	if (FAILED(hr))
	{
		return hr;
	}

	CComPtr<IDXGIFactory2> factory;
	hr = adapter->GetParent(IID_PPV_ARGS(&factory));
	if (FAILED(hr))
	{
		return hr;
	}


	DXGI_SWAP_CHAIN_DESC1 swapdesc{};
	swapdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	swapdesc.SampleDesc.Count = 1;
	swapdesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapdesc.BufferCount = 2;
	swapdesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

	hr = factory->CreateSwapChainForHwnd(d3device, hwnd, &swapdesc, nullptr, nullptr, &m_swapChain);
	if (FAILED(hr))
	{
		return hr;
	}

	CComPtr<ID2D1Device2> device;
	hr = m_factory->CreateDevice(dxdevice, &device);
	if (FAILED(hr))
	{
		return hr;
	}

	hr = device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &m_dc);
	if (FAILED(hr))
	{
		return hr;
	}

	CComPtr<IDXGISurface> surface;
	hr = m_swapChain->GetBuffer(0, IID_PPV_ARGS(&surface));
	if (FAILED(hr))
	{
		return hr;
	}

	const D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
		D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
		D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE)
	);

	CComPtr<ID2D1Bitmap1> bitmap;
	hr = m_dc->CreateBitmapFromDxgiSurface(surface, props, &bitmap);
	if (FAILED(hr))
	{
		return hr;
	}

	m_dc->SetTarget(bitmap);

	const UINT dpi = GetDpiForWindow(hwnd);
	m_dc->SetDpi(dpi, dpi);
	m_size = m_dc->GetSize();

	hr = m_dc->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &m_brush);
	if (FAILED(hr))
	{
		return hr;
	}

	return S_OK;
}