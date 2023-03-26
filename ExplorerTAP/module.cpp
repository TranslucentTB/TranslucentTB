#include <combaseapi.h>
#include <RpcProxy.h>
#include "winrt.hpp"

#include "tap.hpp"
#include "simplefactory.hpp"

extern "C"
{
	_Check_return_ HRESULT STDAPICALLTYPE DLLGETCLASSOBJECT_ENTRY(_In_ REFCLSID rclsid, _In_ REFIID riid, _Outptr_ void** ppv);
	HRESULT STDAPICALLTYPE DLLCANUNLOADNOW_ENTRY();
}

_Use_decl_annotations_ STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv) try
{
	if (rclsid == CLSID_ExplorerTAP)
	{
		*ppv = nullptr;
		return winrt::make<SimpleFactory<ExplorerTAP>>().as(riid, ppv);
	}
	else
	{
		return DLLGETCLASSOBJECT_ENTRY(rclsid, riid, ppv);
	}
}
catch (...)
{
	return winrt::to_hresult();
}

_Use_decl_annotations_ STDAPI DllCanUnloadNow(void)
{
	if (DLLCANUNLOADNOW_ENTRY() == S_FALSE)
	{
		return S_FALSE;
	}
	else if (winrt::get_module_lock())
	{
		return S_FALSE;
	}
	else
	{
		winrt::clear_factory_cache();
		return S_OK;
	}
}
