#pragma once
#include <combaseapi.h>
#include <typeinfo>
#include <wrl/client.h>

#include "ttberror.hpp"
#include "util.hpp"
#include "win32.hpp"

template<typename T>
class ClassicComPtr : public Microsoft::WRL::ComPtr<T> {
public:
	inline void CreateInstance(REFCLSID rclsid, IUnknown *pUnkOuter = nullptr, DWORD dwClsContext = CLSCTX_INPROC_SERVER) throw()
	{
		const HRESULT hr = CoCreateInstance(rclsid, pUnkOuter, dwClsContext, IID_PPV_ARGS(this->ReleaseAndGetAddressOf()));
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
	}

	inline ClassicComPtr(REFCLSID rclsid, IUnknown *pUnkOuter = nullptr, DWORD dwClsContext = CLSCTX_INPROC_SERVER) throw() : Microsoft::WRL::ComPtr<T>()
	{
		CreateInstance(rclsid, pUnkOuter, dwClsContext);
	}
};