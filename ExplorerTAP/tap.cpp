#include "tap.hpp"

HRESULT ExplorerTAP::SetSite(IUnknown *pUnkSite) try
{
	if (visualTreeService && visualTreeWatcher)
	{
		winrt::check_hresult(visualTreeService->UnadviseVisualTreeChange(visualTreeWatcher.get()));
		visualTreeWatcher->SetXamlDiagnostics(nullptr);
	}

	visualTreeService = FromIUnknown<IVisualTreeService3>(pUnkSite);

	if (visualTreeService)
	{
		if (!visualTreeWatcher)
		{
			visualTreeWatcher = winrt::make_self<VisualTreeWatcher>();
		}

		visualTreeWatcher->SetXamlDiagnostics(visualTreeService.as<IXamlDiagnostics>());
		winrt::check_hresult(visualTreeService->AdviseVisualTreeChange(visualTreeWatcher.get()));
	}

	return S_OK;
}
catch (...)
{
	return winrt::to_hresult();
}

HRESULT ExplorerTAP::GetSite(REFIID riid, void **ppvSite) noexcept
{
	return visualTreeService.as(riid, ppvSite);
}
