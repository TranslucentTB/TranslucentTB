#pragma once
#include <ocidl.h>
#include <xamlOM.h>
#include "winrt.hpp"
#include <wil/resource.h>

#include "ExplorerTAP.h"
#include "visualtreewatcher.hpp"

class TAPSite : public winrt::implements<TAPSite, IObjectWithSite, winrt::non_agile>
{
public:
	static wil::unique_event_nothrow GetReadyEvent();

private:
	HRESULT STDMETHODCALLTYPE SetSite(IUnknown* pUnkSite) override;
	HRESULT STDMETHODCALLTYPE GetSite(REFIID riid, void** ppvSite) noexcept override;

	static winrt::weak_ref<VisualTreeWatcher> s_VisualTreeWatcher;

	winrt::com_ptr<IUnknown> site;
};
