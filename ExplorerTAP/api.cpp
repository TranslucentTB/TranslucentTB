#include "api.hpp"
#include <libloaderapi.h>
#include <xamlOM.h>
#include <wil/resource.h>

#include "tap.hpp"
#include "win32.hpp"
#include "util/string_macros.hpp"

using PFN_INITIALIZE_XAML_DIAGNOSTICS_EX = decltype(&InitializeXamlDiagnosticsEx);

HRESULT InjectExplorerTAP(DWORD pid)
{
	const auto [location, hr] = win32::GetDllLocation(wil::GetModuleInstanceHandle());
	if (FAILED(hr)) [[unlikely]]
	{
		return hr;
	}

	const wil::unique_hmodule wux(LoadLibraryEx(L"Windows.UI.Xaml.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32));
	if (!wux)
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	if (const auto ixde = reinterpret_cast<PFN_INITIALIZE_XAML_DIAGNOSTICS_EX>(GetProcAddress(wux.get(), UTIL_STRINGIFY_UTF8(InitializeXamlDiagnosticsEx))))
	{
		return ixde(L"VisualDiagConnection1", pid, nullptr, location.c_str(), CLSID_ExplorerTAP, nullptr);
	}
	else
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}
}
