#include "api.hpp"
#include <libloaderapi.h>
#include <wil/resource.h>

#include "constants.hpp"
#include "tapsite.hpp"
#include "win32.hpp"
#include "taskbarappearanceservice.hpp"
#include "util/string_macros.hpp"

using PFN_INITIALIZE_XAML_DIAGNOSTICS_EX = decltype(&InitializeXamlDiagnosticsEx);

HRESULT InjectExplorerTAP(DWORD pid, REFIID riid, LPVOID* ppv) try
{
	TaskbarAppearanceService::InstallProxyStub();

	winrt::com_ptr<IUnknown> service;
	HRESULT hr = GetActiveObject(CLSID_TaskbarAppearanceService, nullptr, service.put());

	if (hr == MK_E_UNAVAILABLE)
	{
		const auto event = TAPSite::GetReadyEvent();

		const auto [location, hr2] = win32::GetDllLocation(wil::GetModuleInstanceHandle());
		if (FAILED(hr2)) [[unlikely]]
		{
			return hr2;
		}

		const wil::unique_hmodule wux(LoadLibraryEx(L"Windows.UI.Xaml.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32));
		if (!wux) [[unlikely]]
		{
			return HRESULT_FROM_WIN32(GetLastError());
		}

		const auto ixde = reinterpret_cast<PFN_INITIALIZE_XAML_DIAGNOSTICS_EX>(GetProcAddress(wux.get(), UTIL_STRINGIFY_UTF8(InitializeXamlDiagnosticsEx)));
		if (!ixde) [[unlikely]]
		{
			return HRESULT_FROM_WIN32(GetLastError());
		}

		uint8_t attempts = 0;
		do
		{
			// We need this to exist because XAML Diagnostics can only be initialized once per thread
			// future calls simply return S_OK without doing anything.
			// But we need to be able to initialize it again if Explorer restarts. So we create a thread
			// that is discardable to do the initialization from.
			std::thread([&hr, ixde, pid, &location]
			{
				hr = ixde(L"VisualDiagConnection1", pid, nullptr, location.c_str(), CLSID_TAPSite, nullptr);
			}).join();

			if (SUCCEEDED(hr))
			{
				break;
			}
			else
			{
				++attempts;
				Sleep(500);
			}
		} while (FAILED(hr) && attempts < 60);
		// 60 * 500ms = 30s

		if (FAILED(hr)) [[unlikely]]
		{
			return hr;
		}

		static constexpr DWORD TIMEOUT =
#ifdef _DEBUG
			// do not timeout on debug builds, to allow debugging the DLL while it's loading in explorer
			INFINITE;
#else
			5000;
#endif

		if (!event.wait(TIMEOUT)) [[unlikely]]
		{
			return HRESULT_FROM_WIN32(WAIT_TIMEOUT);
		}

		hr = GetActiveObject(CLSID_TaskbarAppearanceService, nullptr, service.put());
	}

	if (FAILED(hr)) [[unlikely]]
	{
		return hr;
	}

	DWORD version = 0;
	hr = service.as<IVersionedApi>()->GetVersion(&version);
	if (SUCCEEDED(hr))
	{
		if (version != TAP_API_VERSION)
		{
			return HRESULT_FROM_WIN32(ERROR_PRODUCT_VERSION);
		}
	}
	else
	{
		return hr;
	}

	return service.as(riid, ppv);
}
catch (...)
{
	return winrt::to_hresult();
}
