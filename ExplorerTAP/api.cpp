#include "api.hpp"
#include <libloaderapi.h>
#include <xamlOM.h>
#include <wil/resource.h>
#include <wil/win32_helpers.h>

#include "tap.hpp"
#include "win32.hpp"
#include "util/string_macros.hpp"

using PFN_INITIALIZE_XAML_DIAGNOSTICS_EX = decltype(&InitializeXamlDiagnosticsEx);

HRESULT InjectExplorerTAP(DWORD pid)
{
	std::wstring location;
	location.resize(wil::max_path_length);
	if (auto size = GetModuleFileName(wil::GetModuleInstanceHandle(), location.data(), static_cast<DWORD>(location.size()) + 1))
	{
		if (size == location.size() + 1 && GetLastError() == ERROR_INSUFFICIENT_BUFFER) [[unlikely]]
		{
			location.resize(wil::max_extended_path_length);
			size = GetModuleFileName(wil::GetModuleInstanceHandle(), location.data(), static_cast<DWORD>(location.size()) + 1);
			
			if (!size) [[unlikely]]
			{
				return HRESULT_FROM_WIN32(GetLastError());
			}
		}

		location.resize(size);
	}
	else
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	const wil::unique_hmodule wux(LoadLibraryEx(L"Windows.UI.Xaml.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32));
	if (!wux)
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	if (const auto ixde = reinterpret_cast<PFN_INITIALIZE_XAML_DIAGNOSTICS_EX>(GetProcAddress(wux.get(), UTIL_STRINGIFY_UTF8(InitializeXamlDiagnosticsEx))))
	{
		return ixde(L"VisualDiagConnection1", pid, L"C:\\Program Files (x86)\\Windows Kits\\10\\bin\\x64\\XamlDiagnostics\\xamldiagnostics.dll", location.c_str(), CLSID_ExplorerTAP, nullptr);
	}
	else
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}
}
