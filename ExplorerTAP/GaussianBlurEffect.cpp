#pragma warning ( disable: 4100 26447 26490 26429 26818 )
#include "GaussianBlurEffect.h"

HRESULT __stdcall GaussianBlurEffect::GetEffectId(GUID* id) noexcept
{
	if (id == nullptr)
		return E_INVALIDARG;

	*id = CLSID_D2D1GaussianBlur;
	return S_OK;
}

HRESULT __stdcall GaussianBlurEffect::GetNamedPropertyMapping(LPCWSTR name, UINT* index, awge::GRAPHICS_EFFECT_PROPERTY_MAPPING* mapping) noexcept
{
	if (index == nullptr || mapping == nullptr)
		return E_INVALIDARG;

	if (_wcsicmp(name, L"BlurAmount") == 0)
	{
		*index = D2D1_GAUSSIANBLUR_PROP_STANDARD_DEVIATION;
		*mapping = awge::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT;

		return S_OK;
	}
	else if (_wcsicmp(name, L"Optimization") == 0)
	{
		*index = D2D1_GAUSSIANBLUR_PROP_OPTIMIZATION;
		*mapping = awge::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT;

		return S_OK;
	}
	else if (_wcsicmp(name, L"BorderMode") == 0)
	{
		*index = D2D1_GAUSSIANBLUR_PROP_BORDER_MODE;
		*mapping = awge::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT;

		return S_OK;
	}

	return E_INVALIDARG;
}

HRESULT __stdcall GaussianBlurEffect::GetPropertyCount(UINT* count) noexcept
{
	if (count == nullptr)
		return E_INVALIDARG;

	*count = 3;
	return S_OK;
}

HRESULT __stdcall GaussianBlurEffect::GetProperty(UINT index, ABI::Windows::Foundation::IPropertyValue** value) noexcept
{
	if (value == nullptr)
		return E_INVALIDARG;

	switch (index)
	{
		case D2D1_GAUSSIANBLUR_PROP_STANDARD_DEVIATION:
		{
			reinterpret_cast<wf::IPropertyValue&>(*value) = wf::PropertyValue::CreateSingle(BlurAmount).as<wf::IPropertyValue>();
			break;
		}
		case D2D1_GAUSSIANBLUR_PROP_OPTIMIZATION:
		{
			reinterpret_cast<wf::IPropertyValue&>(*value) = wf::PropertyValue::CreateUInt32((UINT32)Optimization).as<wf::IPropertyValue>();
			break;
		}
		case D2D1_GAUSSIANBLUR_PROP_BORDER_MODE:
		{
			reinterpret_cast<wf::IPropertyValue&>(*value) = wf::PropertyValue::CreateUInt32((UINT32)BorderMode).as<wf::IPropertyValue>();
			break;
		}
	}

	return S_OK;
}

HRESULT __stdcall GaussianBlurEffect::GetSource(UINT index, awge::IGraphicsEffectSource** source) noexcept
{
	if (source == nullptr)
		return E_INVALIDARG;

	reinterpret_cast<wge::IGraphicsEffectSource&>(*source) = Source;
	return S_OK;
}

HRESULT __stdcall GaussianBlurEffect::GetSourceCount(UINT* count) noexcept
{
	if (count == nullptr)
		return E_INVALIDARG;

	*count = 1;
	return S_OK;
}

winrt::hstring GaussianBlurEffect::Name()
{
	return m_name;
}

void GaussianBlurEffect::Name(winrt::hstring name)
{
	m_name = name;
}
