#include "api.hpp"
#include <libloaderapi.h>
#include <xamlOM.h>
#include <wil/resource.h>

#include "taskbarappearanceservice.hpp"
#include "tap.hpp"
#include "win32.hpp"
#include "util/string_macros.hpp"

using PFN_INITIALIZE_XAML_DIAGNOSTICS_EX = decltype(&InitializeXamlDiagnosticsEx);

HRESULT InjectExplorerTAP(DWORD pid, REFIID riid, LPVOID* ppv) noexcept
{
	try
	{
		TaskbarAppearanceService::InstallProxyStub();
	}
	catch (...)
	{
		return winrt::to_hresult();
	}

	const auto [location, hr] = win32::GetDllLocation(wil::GetModuleInstanceHandle());
	if (FAILED(hr)) [[unlikely]]
	{
		return hr;
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

	const HRESULT hr2 = ixde(L"VisualDiagConnection1", pid, nullptr, location.c_str(), CLSID_ExplorerTAP, nullptr);
	if (FAILED(hr2)) [[unlikely]]
	{
		return hr2;
	}

	return CoCreateInstance(CLSID_TaskbarAppearanceService, nullptr, CLSCTX_LOCAL_SERVER, riid, ppv);
}
