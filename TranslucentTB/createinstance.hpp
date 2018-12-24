#pragma once
#include <combaseapi.h>
#include <typeinfo>
#include <winrt/base.h>

#include "ttberror.hpp"
#include "util.hpp"
#include "win32.hpp"

template<typename T>
inline winrt::com_ptr<T> create_instance(REFCLSID rclsid, IUnknown *pUnkOuter = nullptr, CLSCTX dwClsContext = CLSCTX_INPROC_SERVER) noexcept
{
	static_assert(std::is_base_of_v<IUnknown, T>, "T is not a COM interface.");

	winrt::com_ptr<T> ptr;
	const HRESULT hr = CoCreateInstance(rclsid, pUnkOuter, dwClsContext, IID_PPV_ARGS(ptr.put()));
	if (FAILED(hr))
	{
		std::wstring iname = win32::CharToWchar(typeid(T).name());
		if (!iname.empty())
		{
			Util::RemovePrefixInplace(iname, L"struct ");
			Util::RemovePrefixInplace(iname, L"class ");
		}
		else
		{
			iname = L"[failed to convert interface name to UTF-16]";
		}
		ErrorHandle(hr, Error::Level::Log, (L"Failed to create instance of COM interface " + iname + L'.').c_str());
	}

	return ptr;
}