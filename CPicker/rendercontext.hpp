#pragma once
#include <d2d1_3.h>
#include <d3d11.h>
#include <dcomp.h>
#include <dxgi1_2.h>
#include <type_traits>
#include <utility>
#include <winrt/base.h>

#include "scolour.hpp"

class RenderContext {
private:
	static HRESULT CreateDevice(D3D_DRIVER_TYPE type, ID3D11Device **device, ID3D11DeviceContext **context);
	static bool NeedsAlpha(HWND hwnd);

	HRESULT CreateNonAlphaSwapChain(HWND hwnd, IDXGIFactory2 *factory, ID3D11Device *device);
	HRESULT CreateAlphaSwapChain(HWND hwnd, IDXGIFactory2 *factory, ID3D11Device *device, IDXGIDevice1 *dxgidevice);

	ID2D1Factory3 *m_factory;
	winrt::com_ptr<IDXGISwapChain1> m_swapChain;
	winrt::com_ptr<ID3D11DeviceContext> m_d3dc;
	winrt::com_ptr<IDCompositionDevice> m_compDevice;
	winrt::com_ptr<IDCompositionTarget> m_compTarget;

protected:
	winrt::com_ptr<ID2D1DeviceContext2> m_dc;
	winrt::com_ptr<ID2D1SolidColorBrush> m_brush;
	D2D1_SIZE_F m_size;

	HRESULT CreateGradient(ID2D1LinearGradientBrush **brush, const D2D1_COLOR_F &top, const D2D1_COLOR_F &bottom);

	class DrawContext {
	private:
		bool m_needsEnd;
		ID2D1RenderTarget *m_target;
		IDXGISwapChain *m_swapChain;
		IDCompositionDevice *m_compDevice;
		inline DrawContext(ID2D1RenderTarget *target, IDXGISwapChain *swapChain, IDCompositionDevice *compDevice) :
			m_needsEnd(true),
			m_target(target),
			m_swapChain(swapChain),
			m_compDevice(compDevice)
		{
			m_target->BeginDraw();
		}

		inline void MoveToSelf(DrawContext &&other) noexcept
		{
			if (this != &other)
			{
				m_needsEnd = std::exchange(other.m_needsEnd, false);

				m_target = std::exchange(other.m_target, nullptr);
				m_swapChain = std::exchange(other.m_swapChain, nullptr);
				m_compDevice = std::exchange(other.m_compDevice, nullptr);
			}
		}

		friend class RenderContext;

	public:
		inline DrawContext(const DrawContext &) = delete;
		inline DrawContext &operator =(const DrawContext &) = delete;

		inline DrawContext(DrawContext &&other) noexcept
		{
			MoveToSelf(std::forward<DrawContext>(other));
		}

		inline DrawContext &operator =(DrawContext &&other) noexcept
		{
			MoveToSelf(std::forward<DrawContext>(other));
			return *this;
		}

		// Usually you want to manually call EndDraw to get HRESULT error reporting, but if needs arise
		// (for example, when returning because of an error while drawing) the destructor will do it and drop errors.
		inline HRESULT EndDraw()
		{
			if (m_needsEnd)
			{
				m_needsEnd = false;
				HRESULT hr = m_target->EndDraw();
				if (FAILED(hr))
				{
					return hr;
				}

				hr = m_swapChain->Present(0, 0);
				if (FAILED(hr))
				{
					return hr;
				}

				if (m_compDevice)
				{
					hr = m_compDevice->Commit();
					if (FAILED(hr))
					{
						return hr;
					}
				}
			}

			return S_OK;
		}

		inline ~DrawContext()
		{
			EndDraw();
		}
	};

	inline DrawContext BeginDraw()
	{
		return DrawContext(m_dc.get(), m_swapChain.get(), m_compDevice.get());
	}

	virtual void Destroy();

public:
	virtual HRESULT Refresh(HWND hwnd);
	virtual HRESULT Draw(HWND hDlg, const SColourF &col) = 0;

	inline explicit RenderContext(ID2D1Factory3 *factory) : m_factory(factory), m_size() { }

	inline RenderContext(const RenderContext &) = delete;
	inline RenderContext &operator =(const RenderContext &) = delete;
};