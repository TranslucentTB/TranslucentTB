#include <combaseapi.h>
#include "winrt.hpp"

#include "tap.hpp"
#include "simplefactory.hpp"

_Use_decl_annotations_ STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv) try
{
	*ppv = nullptr;

	if (rclsid == CLSID_ExplorerTAP)
	{
		return winrt::make<SimpleFactory<ExplorerTAP>>().as(riid, ppv);
	}
	else
	{
		return CLASS_E_CLASSNOTAVAILABLE;
	}
}
catch (...)
{
	return winrt::to_hresult();
}

_Use_decl_annotations_ STDAPI DllCanUnloadNow(void)
{
	if (winrt::get_module_lock())
	{
		return S_FALSE;
	}
	else
	{

		return S_OK;
	}
}
