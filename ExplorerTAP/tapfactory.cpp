#include "tapfactory.hpp"

#include "tap.hpp"

HRESULT TAPFactory::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObject) try
{
	*ppvObject = nullptr;

	if (!pUnkOuter)
	{
		return winrt::make<ExplorerTAP>().as(riid, ppvObject);
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

HRESULT TAPFactory::LockServer(BOOL) noexcept
{
	return S_OK;
}
