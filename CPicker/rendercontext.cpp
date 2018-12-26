#include "rendercontext.hpp"

HRESULT RenderContext::CreateDevice(IDXGIAdapter *adapter, D3D_DRIVER_TYPE type, ID3D11Device **device, ID3D11DeviceContext **context)
{
	UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
	flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	return D3D11CreateDevice(adapter, type, nullptr, flags, nullptr, 0, D3D11_SDK_VERSION, device, nullptr, context);
}

HRESULT RenderContext::CreateNonAlphaSwapChain(HWND hwnd, IDXGIFactory2 *factory, ID3D11Device *device)
{
	DXGI_SWAP_CHAIN_DESC1 swapdesc{};
	swapdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	swapdesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
	swapdesc.SampleDesc.Count = 1;
	swapdesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapdesc.BufferCount = 2;
	swapdesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

	return factory->CreateSwapChainForHwnd(device, hwnd, &swapdesc, nullptr, nullptr, m_swapChain.put());
}

HRESULT RenderContext::CreateAlphaSwapChain(HWND hwnd, IDXGIFactory2 *factory, ID3D11Device *device, IDXGIDevice1 *dxgidevice)
{
	HRESULT hr = DCompositionCreateDevice(dxgidevice, IID_PPV_ARGS(m_compDevice.put()));
	if (FAILED(hr))
	{
		return hr;
	}

	hr = m_compDevice->CreateTargetForHwnd(hwnd, true, m_compTarget.put());
	if (FAILED(hr))
	{
		return hr;
	}

	winrt::com_ptr<IDCompositionVisual> compVisual;
	hr = m_compDevice->CreateVisual(compVisual.put());
	if (FAILED(hr))
	{
		return hr;
	}

	RECT rect;
	if (!GetClientRect(hwnd, &rect))
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	DXGI_SWAP_CHAIN_DESC1 swapdesc{};
	swapdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	swapdesc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;
	swapdesc.SampleDesc.Count = 1;
	swapdesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapdesc.BufferCount = 2;
	swapdesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	swapdesc.Width = rect.right - rect.left;
	swapdesc.Height = rect.bottom - rect.top;

	hr = factory->CreateSwapChainForComposition(device, &swapdesc, nullptr, m_swapChain.put());
	if (FAILED(hr))
	{
		return hr;
	}

	hr = compVisual->SetContent(m_swapChain.get());
	if (FAILED(hr))
	{
		return hr;
	}

	return m_compTarget->SetRoot(compVisual.get());
}

HRESULT RenderContext::CreateDeviceContext(HWND hwnd, bool needsAlpha)
{
	HRESULT hr = m_d2device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, m_dc.put());
	if (FAILED(hr))
	{
		return hr;
	}

	winrt::com_ptr<IDXGISurface> surface;
	hr = m_swapChain->GetBuffer(0, IID_PPV_ARGS(surface.put()));
	if (FAILED(hr))
	{
		return hr;
	}

	const UINT dpi = GetDpiForWindow(hwnd);
	if (!dpi)
	{
		return HRESULT_FROM_WIN32(ERROR_INVALID_WINDOW_HANDLE);
	}

	const D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
		D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
		D2D1::PixelFormat(
			DXGI_FORMAT_B8G8R8A8_UNORM,
			needsAlpha ? D2D1_ALPHA_MODE_PREMULTIPLIED : D2D1_ALPHA_MODE_IGNORE
		),
		dpi,
		dpi
	);

	winrt::com_ptr<ID2D1Bitmap1> bitmap;
	hr = m_dc->CreateBitmapFromDxgiSurface(surface.get(), props, bitmap.put());
	if (FAILED(hr))
	{
		return hr;
	}

	m_dc->SetTarget(bitmap.get());

	m_size = m_dc->GetSize();

	return S_OK;
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
	winrt::com_ptr<ID2D1GradientStopCollection> pGradientStops;
	hr = m_dc->CreateGradientStopCollection(
		gradientStops,
		2,
		D2D1_GAMMA_1_0,
		D2D1_EXTEND_MODE_CLAMP,
		pGradientStops.put()
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
		pGradientStops.get(),
		brush
	);

	if (FAILED(hr))
	{
		return hr;
	}

	return S_OK;
}

void RenderContext::Destroy()
{
	m_compDevice = nullptr;
	m_compTarget = nullptr;
	m_swapChain = nullptr;
	m_d2device = nullptr;
	m_dc = nullptr;
	m_brush = nullptr;

	// https://docs.microsoft.com/en-us/windows/desktop/api/d3d11/nf-d3d11-id3d11devicecontext-flush#Defer_Issues_with_Flip
	if (m_d3dc)
	{
		m_d3dc->ClearState();
		m_d3dc->Flush();
		m_d3dc = nullptr;
	}
}

HRESULT RenderContext::Refresh(HWND hwnd)
{
	Destroy();
	HRESULT hr;

	winrt::com_ptr<IDXGIFactory2> factory;
	UINT flags = 0;
#ifdef _DEBUG
	flags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
	hr = CreateDXGIFactory2(flags, IID_PPV_ARGS(factory.put()));
	if (FAILED(hr))
	{
		return hr;
	}

	winrt::com_ptr<IDXGIAdapter1> dxgiadapter;
	hr = factory->EnumAdapters1(0, dxgiadapter.put());
	if (FAILED(hr))
	{
		return hr;
	}

	winrt::com_ptr<ID3D11Device> d3device;
	hr = CreateDevice(dxgiadapter.get(), D3D_DRIVER_TYPE_UNKNOWN, d3device.put(), m_d3dc.put());
	if (FAILED(hr))
	{
		hr = CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, d3device.put(), m_d3dc.put());
		if (FAILED(hr))
		{
			return hr;
		}
	}

	winrt::com_ptr<IDXGIDevice1> dxdevice;
	hr = d3device->QueryInterface(dxdevice.put());
	if (FAILED(hr))
	{
		return hr;
	}

	hr = dxdevice->SetMaximumFrameLatency(1);
	if (FAILED(hr))
	{
		return hr;
	}

	const bool needsAlpha = NeedsAlpha(hwnd);
	hr = needsAlpha
		? CreateAlphaSwapChain(hwnd, factory.get(), d3device.get(), dxdevice.get())
		: CreateNonAlphaSwapChain(hwnd, factory.get(), d3device.get());
	if (FAILED(hr))
	{
		return hr;
	}

	hr = m_factory->CreateDevice(dxdevice.get(), m_d2device.put());
	if (FAILED(hr))
	{
		return hr;
	}

	hr = CreateDeviceContext(hwnd, needsAlpha);
	if (FAILED(hr))
	{
		return hr;
	}

	hr = m_dc->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), m_brush.put());
	if (FAILED(hr))
	{
		return hr;
	}

	return ResizeResources();
}

HRESULT RenderContext::OnDpiChange(HWND hwnd)
{
	const UINT dpi = GetDpiForWindow(hwnd);
	if (!dpi)
	{
		return HRESULT_FROM_WIN32(ERROR_INVALID_WINDOW_HANDLE);
	}

	float dpiX, dpiY;
	m_dc->GetDpi(&dpiX, &dpiY);

	if (static_cast<UINT>(std::roundf(dpiX)) != dpi || static_cast<UINT>(std::roundf(dpiY)) != dpi)
	{
		m_dc->SetDpi(dpi, dpi);

		// Not setting m_size because a DPI change is immediatly followed by a size change which does it.
	}

	return S_OK;
}

HRESULT RenderContext::OnSizeChange(HWND hwnd)
{
	RECT rect;
	if (!GetClientRect(hwnd, &rect))
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	const D2D1_SIZE_U size = m_dc->GetPixelSize();
	const UINT width = rect.right - rect.left;
	const UINT height = rect.bottom - rect.top;

	if (width != size.width || height != size.height)
	{
		m_dc = nullptr;
		m_d3dc->Flush();
		m_d3dc->ClearState();

		HRESULT hr = m_swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
		if (FAILED(hr))
		{
			return hr;
		}

		hr = CreateDeviceContext(hwnd, NeedsAlpha(hwnd));
		if (FAILED(hr))
		{
			return hr;
		}

		return ResizeResources();
	}

	return S_OK;
}