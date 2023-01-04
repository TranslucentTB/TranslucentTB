#pragma once
#include <Unknwn.h>
#include "winrt.hpp"

struct TAPFactory : winrt::implements<TAPFactory, IClassFactory>
{
	HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObject) override;
	HRESULT STDMETHODCALLTYPE LockServer(BOOL) noexcept override;
};
