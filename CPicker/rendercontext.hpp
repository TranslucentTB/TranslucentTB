#pragma once
#include <d2d1_3.h>
#include <dxgi1_2.h>
#include <d3d11.h>
#include <type_traits>
#include <utility>
#include <wrl/client.h>

#include "scolour.hpp"

class RenderContext {
protected:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

private:
	static HRESULT CreateDevice(const D3D_DRIVER_TYPE &type, ID3D11Device **device, ID3D11DeviceContext **context);

	ID2D1Factory3 *const m_factory;
	ComPtr<IDXGISwapChain1> m_swapChain;
	ComPtr<ID3D11DeviceContext> m_d3dc;

protected:
	ComPtr<ID2D1DeviceContext2> m_dc;
	ComPtr<ID2D1SolidColorBrush> m_brush;
	D2D1_SIZE_F m_size;

	HRESULT CreateGradient(ID2D1LinearGradientBrush **const brush, const D2D1_COLOR_F &top, const D2D1_COLOR_F &bottom);

	class DrawContext {
	private:
		bool m_needsEnd;
		ID2D1RenderTarget *m_target;
		IDXGISwapChain *m_swapChain;
		inline DrawContext(ID2D1RenderTarget *const target, IDXGISwapChain *const swapChain) : m_needsEnd(true), m_target(target), m_swapChain(swapChain)
		{
			m_target->BeginDraw();
		}

		inline void MoveToSelf(DrawContext &&other) noexcept
		{
			m_needsEnd = std::exchange(other.m_needsEnd, false);

			m_target = std::exchange(other.m_target, nullptr);
			m_swapChain = std::exchange(other.m_swapChain, nullptr);
		}

		friend RenderContext;

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
		return DrawContext(m_dc.Get(), m_swapChain.Get());
	}

public:
	virtual HRESULT Refresh(HWND hwnd);
	virtual HRESULT Draw(const HWND hDlg, const SColourF &col, const SColourF &old) = 0;

	explicit inline RenderContext(ID2D1Factory3 *const factory) : m_factory(factory) { }

	inline RenderContext(const RenderContext &) = delete;
	inline RenderContext &operator =(const RenderContext &) = delete;
};