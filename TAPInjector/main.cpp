#include <Windows.h>
#include <objbase.h>
#include <shellapi.h>
#include <xamlOM.h>
#include <wil/resource.h>
#include <string>
#include <vector>

#include "util/null_terminated_string_view.hpp"
#include "util/numbers.hpp"
#include "util/string_macros.hpp"

using PFN_INITIALIZE_XAML_DIAGNOSTICS_EX = decltype(&InitializeXamlDiagnosticsEx);

// We need this to exist because XAML Diagnostics can only be initialized once per process
// future calls simply return S_OK without doing anything.
// But we need to be able to initialize it again if Explorer restarts. So we create a process
// that is discardable to do the initialization from.
_Use_decl_annotations_ int WINAPI wWinMain(HINSTANCE, HINSTANCE, wchar_t*, int)
{
	int argc = 0;
	wil::unique_hlocal_ptr<wchar_t*> argv(CommandLineToArgvW(GetCommandLine(), &argc));
	if (argv)
	{
		std::vector<Util::null_terminated_wstring_view> args(argv.get(), argv.get() + argc);

		if (args.size() >= 4)
		{
			DWORD pid{};
			try
			{
				pid = Util::ParseHexNumber<DWORD>(args[1]);
			}
			catch (...)
			{
				return E_FAIL;
			}

			CLSID clsid{};
			const HRESULT hr = IIDFromString(args[3].c_str(), &clsid);
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

			return ixde(L"VisualDiagConnection1", pid, nullptr, args[2].c_str(), clsid, nullptr);
		}
		else
		{
			return E_BOUNDS;
		}
	}

	return HRESULT_FROM_WIN32(GetLastError());
}
