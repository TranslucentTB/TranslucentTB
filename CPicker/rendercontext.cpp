#include "rendercontext.hpp"

HRESULT RenderContext::CreateDevice(const D3D_DRIVER_TYPE &type, ID3D11Device **device, ID3D11DeviceContext **context)
{
	UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
	flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	return D3D11CreateDevice(nullptr, type, nullptr, flags, nullptr, 0, D3D11_SDK_VERSION, device, nullptr, context);
}

HRESULT RenderContext::CreateGradient(ID2D1LinearGradientBrush **brush, const D2D1_COLOR_F &top, const D2D1_COLOR_F &bottom)
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
	ComPtr<ID2D1GradientStopCollection> pGradientStops;
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
		pGradientStops.Get(),
		brush
	);

	if (FAILED(hr))
	{
		return hr;
	}

	return S_OK;
}

HRESULT RenderContext::Refresh(HWND hwnd)
{
	HRESULT hr;

	ComPtr<ID3D11DeviceContext> oldd3dc = m_d3dc;

	ComPtr<ID3D11Device> d3device;
	hr = CreateDevice(D3D_DRIVER_TYPE_HARDWARE, &d3device, &m_d3dc);
	if (FAILED(hr))
	{
		return hr;
	}

	ComPtr<IDXGIDevice1> dxdevice;
	hr = d3device.As(&dxdevice);
	if (FAILED(hr))
	{
		return hr;
	}

	hr = dxdevice->SetMaximumFrameLatency(1);
	if (FAILED(hr))
	{
		return hr;
	}

	ComPtr<IDXGIAdapter> adapter;
	hr = dxdevice->GetAdapter(&adapter);
	if (FAILED(hr))
	{
		return hr;
	}

	ComPtr<IDXGIFactory2> factory;
	hr = adapter->GetParent(IID_PPV_ARGS(&factory));
	if (FAILED(hr))
	{
		return hr;
	}

	ComPtr<ID2D1Device2> device;
	hr = m_factory->CreateDevice(dxdevice.Get(), &device);
	if (FAILED(hr))
	{
		return hr;
	}

	hr = device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &m_dc);
	if (FAILED(hr))
	{
		return hr;
	}

	// https://docs.microsoft.com/en-us/windows/desktop/api/d3d11/nf-d3d11-id3d11devicecontext-flush#Defer_Issues_with_Flip
	if (oldd3dc.Get() != nullptr)
	{
		oldd3dc->ClearState();
		oldd3dc->Flush();
	}

	DXGI_SWAP_CHAIN_DESC1 swapdesc{};
	swapdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	swapdesc.SampleDesc.Count = 1;
	swapdesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapdesc.BufferCount = 2;
	swapdesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

	hr = factory->CreateSwapChainForHwnd(d3device.Get(), hwnd, &swapdesc, nullptr, nullptr, &m_swapChain);
	if (FAILED(hr))
	{
		return hr;
	}

	ComPtr<IDXGISurface> surface;
	hr = m_swapChain->GetBuffer(0, IID_PPV_ARGS(&surface));
	if (FAILED(hr))
	{
		return hr;
	}

	const UINT dpi = GetDpiForWindow(hwnd);

	const D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
		D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
		D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE),
		dpi,
		dpi
	);

	ComPtr<ID2D1Bitmap1> bitmap;
	hr = m_dc->CreateBitmapFromDxgiSurface(surface.Get(), props, &bitmap);
	if (FAILED(hr))
	{
		return hr;
	}

	m_dc->SetTarget(bitmap.Get());

	m_size = m_dc->GetSize();

	hr = m_dc->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &m_brush);
	if (FAILED(hr))
	{
		return hr;
	}

	return S_OK;
}