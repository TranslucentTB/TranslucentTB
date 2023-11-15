#include <combaseapi.h>
#include "winrt.hpp"

#include "tapsite.hpp"
#include "simplefactory.hpp"

_Use_decl_annotations_ STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv) try
{
	if (rclsid == CLSID_TAPSite)
	{
		*ppv = nullptr;
		return winrt::make<SimpleFactory<TAPSite>>().as(riid, ppv);
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
