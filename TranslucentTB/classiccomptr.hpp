#pragma once
#include <combaseapi.h>
#include <typeinfo>
#include <wrl/client.h>

#include "ttberror.hpp"
#include "util.hpp"
#include "win32.hpp"

template<typename T>
class ClassicComPtr : public Microsoft::WRL::ComPtr<T> {

private:
	inline HRESULT InternalCreateInstance(REFCLSID rclsid, IUnknown *pUnkOuter, DWORD dwClsContext) throw()
	{
		return CoCreateInstance(rclsid, pUnkOuter, dwClsContext, __uuidof(T), reinterpret_cast<void **>(this->ReleaseAndGetAddressOf()));
	}

public:
	inline void CreateInstance(REFCLSID rclsid, IUnknown *pUnkOuter = nullptr, DWORD dwClsContext = CLSCTX_ALL) throw()
	{
		const HRESULT hr = InternalCreateInstance(rclsid, pUnkOuter, dwClsContext);
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

	inline ClassicComPtr(REFCLSID rclsid, IUnknown *pUnkOuter = nullptr, DWORD dwClsContext = CLSCTX_ALL) throw() : Microsoft::WRL::ComPtr<T>()
	{
		CreateInstance(rclsid, pUnkOuter, dwClsContext);
	}

};