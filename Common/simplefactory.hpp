#pragma once
#include <Unknwn.h>
#include "winrt.hpp"

template<class T>
struct SimpleFactory : winrt::implements<SimpleFactory<T>, IClassFactory, winrt::non_agile>
{
	HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppvObject) override try
	{
		if (!pUnkOuter)
		{
			*ppvObject = nullptr;
			return winrt::make<T>().as(riid, ppvObject);
		}
		else
		{
			return CLASS_E_NOAGGREGATION;
		}
	}
	catch (...)
	{
		return winrt::to_hresult();
	}

	HRESULT STDMETHODCALLTYPE LockServer(BOOL) noexcept override
	{
		return S_OK;
	}
};
