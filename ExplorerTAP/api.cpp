#include "api.hpp"
#include <libloaderapi.h>
#include <processthreadsapi.h>
#include <detours/detours.h>
#include <wil/resource.h>

#include "constants.hpp"
#include "visualtreewatcher.hpp"
#include "tap.hpp"
#include "win32.hpp"
#include "util/string_macros.hpp"

// derived from https://web.archive.org/web/20190109172835/https://blogs.msdn.microsoft.com/twistylittlepassagesallalike/2011/04/23/everyone-quotes-command-line-arguments-the-wrong-way/
std::wstring EscapeForCommandLine(std::wstring_view argument)
{
	// Unless we're told otherwise, don't quote unless we actually
	// need to do so --- hopefully avoid problems if programs won't
	// parse quotes properly

	if (argument.find_first_of(L" \t\n\v\"") == std::wstring_view::npos)
	{
		return std::wstring(argument);
	}
	else
	{
		std::wstring escapedArgument;
		escapedArgument.push_back(L'"');

		for (auto it = argument.begin(); ; ++it)
		{
			unsigned backslashCount = 0;

			while (it != argument.end() && *it == L'\\')
			{
				++it;
				++backslashCount;
			}

			if (it == argument.end())
			{
				// Escape all backslashes, but let the terminating
				// double quotation mark we add below be interpreted
				// as a metacharacter.

				escapedArgument.append(backslashCount * 2, L'\\');
				break;
			}
			else if (*it == L'"')
			{
				// Escape all backslashes and the following
				// double quotation mark.

				escapedArgument.append(backslashCount * 2 + 1, L'\\');
				escapedArgument.push_back(*it);
			}
			else
			{
				// Backslashes aren't special here.

				escapedArgument.append(backslashCount, L'\\');
				escapedArgument.push_back(*it);
			}
		}

		escapedArgument.push_back(L'"');
		return escapedArgument;
	}
}

HRESULT InjectExplorerTAP(DWORD pid, REFIID riid, LPVOID* ppv) try
{
	VisualTreeWatcher::InstallProxyStub();

	winrt::com_ptr<IUnknown> service;
	HRESULT hr = GetActiveObject(CLSID_VisualTreeWatcher, nullptr, service.put());

	if (hr == MK_E_UNAVAILABLE)
	{
		const auto event = ExplorerTAP::GetReadyEvent();

		const wil::unique_process_handle proc(OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE, false, pid));
		if (!proc) [[unlikely]]
		{
			return HRESULT_FROM_WIN32(GetLastError());
		}

		if (!DetourFindRemotePayload(proc.get(), EXPLORER_PAYLOAD, nullptr))
		{
			static constexpr uint32_t content = 0xDEADBEEF;
			if (!DetourCopyPayloadToProcess(proc.get(), EXPLORER_PAYLOAD, &content, sizeof(content))) [[unlikely]]
			{
				return HRESULT_FROM_WIN32(GetLastError());
			}
		}

		const auto [location, hr2] = win32::GetDllLocation(wil::GetModuleInstanceHandle());
		if (FAILED(hr2)) [[unlikely]]
		{
			return hr2;
		}

		wil::unique_cotaskmem_string clsid;
		hr = StringFromIID(CLSID_ExplorerTAP, clsid.put());
		if (FAILED(hr)) [[unlikely]]
		{
			return hr;
		}

		const auto [exe, hr3] = win32::GetExeLocation();
		if (FAILED(hr3)) [[unlikely]]
		{
			return hr3;
		}

		const auto injector = exe.parent_path() / L"TAPInjector.exe";
		auto commandLine = std::format(L"{} {:#x} {} {}", EscapeForCommandLine(injector.native()), pid, EscapeForCommandLine(location.native()), clsid.get());

		STARTUPINFO si = { sizeof(si) };
		wil::unique_process_information pi;
		if (!CreateProcess(injector.native().c_str(), commandLine.data(), nullptr, nullptr, false, 0, nullptr, nullptr, &si, &pi)) [[unlikely]]
		{
			return HRESULT_FROM_WIN32(GetLastError());
		}

		if (WaitForSingleObject(pi.hProcess, 1000) != WAIT_OBJECT_0) [[unlikely]]
		{
			return HRESULT_FROM_WIN32(WAIT_TIMEOUT);
		}

		DWORD exitCode = 0;
		if (!GetExitCodeProcess(pi.hProcess, &exitCode)) [[unlikely]]
		{
			return HRESULT_FROM_WIN32(GetLastError());
		}

		if (FAILED(static_cast<HRESULT>(exitCode))) [[unlikely]]
		{
			return static_cast<HRESULT>(exitCode);
		}

		if (!event.wait(1000)) [[unlikely]]
		{
			return HRESULT_FROM_WIN32(WAIT_TIMEOUT);
		}

		hr = GetActiveObject(CLSID_VisualTreeWatcher, nullptr, service.put());
		if (FAILED(hr))
		{
			return hr;
		}
	}
	else if (FAILED(hr)) [[unlikely]]
	{
		return hr;
	}

	return service.as(riid, ppv);
}
catch (...)
{
	return winrt::to_hresult();
}
