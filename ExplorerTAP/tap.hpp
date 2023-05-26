#pragma once
#include <ocidl.h>
#include <xamlOM.h>
#include "winrt.hpp"
#include <wil/resource.h>

#include "visualtreewatcher.hpp"

static constexpr CLSID CLSID_ExplorerTAP = { 0x36162bd3, 0x3531, 0x4131, { 0x9b, 0x8b, 0x7f, 0xb1, 0xa9, 0x91, 0xef, 0x51 } };

struct ExplorerTAP : winrt::implements<ExplorerTAP, IObjectWithSite, winrt::non_agile>
{
	HRESULT STDMETHODCALLTYPE SetSite(IUnknown *pUnkSite) override;
	HRESULT STDMETHODCALLTYPE GetSite(REFIID riid, void **ppvSite) noexcept override;

	static wil::unique_event_nothrow GetReadyEvent();

private:
	static winrt::weak_ref<VisualTreeWatcher> s_VisualTreeWatcher;

	winrt::com_ptr<IUnknown> site;
};
