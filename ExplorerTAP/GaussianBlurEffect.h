#pragma once
#include "winrt.hpp"
#include "d2d1effects.h"
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Graphics.Effects.h>
#include <windows.graphics.effects.interop.h>

namespace wge = winrt::Windows::Graphics::Effects;
namespace awge = ABI::Windows::Graphics::Effects;

struct GaussianBlurEffect : winrt::implements<GaussianBlurEffect, wge::IGraphicsEffect, wge::IGraphicsEffectSource, awge::IGraphicsEffectD2D1Interop>
{
public:
	// IGraphicsEffectD2D1Interop
	virtual HRESULT STDMETHODCALLTYPE GetEffectId(GUID* id) noexcept override;
	virtual HRESULT STDMETHODCALLTYPE GetNamedPropertyMapping(LPCWSTR name, UINT* index, awge::GRAPHICS_EFFECT_PROPERTY_MAPPING* mapping) noexcept override;
	virtual HRESULT STDMETHODCALLTYPE GetPropertyCount(UINT* count) noexcept override;
	virtual HRESULT STDMETHODCALLTYPE GetProperty(UINT index, ABI::Windows::Foundation::IPropertyValue** value) noexcept override;
	virtual HRESULT STDMETHODCALLTYPE GetSource(UINT index, awge::IGraphicsEffectSource** source) noexcept override;
	virtual HRESULT STDMETHODCALLTYPE GetSourceCount(UINT* count) noexcept override;

	// IGraphicsEffect
	winrt::hstring Name();
	void Name(winrt::hstring name);

	wge::IGraphicsEffectSource Source;

	float BlurAmount = 3.0f;
	D2D1_GAUSSIANBLUR_OPTIMIZATION Optimization = D2D1_GAUSSIANBLUR_OPTIMIZATION_BALANCED;
	D2D1_BORDER_MODE BorderMode = D2D1_BORDER_MODE_SOFT;
private:
	winrt::hstring m_name = L"GaussianBlurEffect";
};
