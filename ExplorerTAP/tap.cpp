#include "tap.hpp"
#include "constants.hpp"

winrt::weak_ref<VisualTreeWatcher> ExplorerTAP::s_VisualTreeWatcher;

HRESULT ExplorerTAP::SetSite(IUnknown *pUnkSite) try
{
	// only ever 1 VTW at once
	if (s_VisualTreeWatcher.get())
	{
		throw winrt::hresult_illegal_method_call();
	}

	site.copy_from(pUnkSite);

	if (site)
	{
		s_VisualTreeWatcher = winrt::make_self<VisualTreeWatcher>(site);

		wil::unique_event_nothrow readyEvent;
		if (readyEvent.try_open(TAP_READY_EVENT.c_str(), EVENT_MODIFY_STATE))
		{
			readyEvent.SetEvent();
		}
	}

	return S_OK;
}
catch (...)
{
	return winrt::to_hresult();
}

HRESULT ExplorerTAP::GetSite(REFIID riid, void **ppvSite) noexcept
{
	return site.as(riid, ppvSite);
}

wil::unique_event_nothrow ExplorerTAP::GetReadyEvent()
{
	wil::unique_event_nothrow readyEvent;
	winrt::check_hresult(readyEvent.create(wil::EventOptions::None, TAP_READY_EVENT.c_str()));
	return readyEvent;
}
