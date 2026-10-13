#include "api.hpp"
#include <WinBase.h>
#include <detours/detours.h>
#include <libloaderapi.h>
#include <wil/resource.h>

#include "constants.hpp"
#include "tapsite.hpp"
#include "taskbarappearanceservice.hpp"
#include "win32.hpp"

LRESULT CALLBACK CallWndProc(int nCode, WPARAM wParam, LPARAM lParam) noexcept
{
	// Placeholder
	return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

HRESULT InjectExplorerTAP(HWND window, REFIID riid, LPVOID* ppv) try
{
	TaskbarAppearanceService::InstallProxyStub();

	DWORD pid = 0;
	const DWORD tid = GetWindowThreadProcessId(window, &pid);

	wil::unique_process_handle proc(OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, pid));
	if (!proc) [[unlikely]]
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	if (!DetourFindRemotePayload(proc.get(), EXPLORER_PAYLOAD, nullptr))
	{
		proc.reset(OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_WRITE, false, pid));
		if (!proc) [[unlikely]]
		{
			return HRESULT_FROM_WIN32(GetLastError());
		}

		static constexpr uint32_t content = 0xDEADBEEF;
		if (!DetourCopyPayloadToProcess(proc.get(), EXPLORER_PAYLOAD, &content, sizeof(content))) [[unlikely]]
		{
			return HRESULT_FROM_WIN32(GetLastError());
		}
	}

	{
		const auto event = TAPSite::GetReadyEvent();

		// InitializeXamlDiagnosticsEx pins the DLL forever in the target process - so we can discard the hook once it happens
		// this has the bonus effect that if InitializeXamlDiagnosticsEx fails, the DLL gets unloaded.
		wil::unique_hhook hook(SetWindowsHookEx(WH_CALLWNDPROC, CallWndProc, wil::GetModuleInstanceHandle(), tid));
		if (!hook)
		{
			return HRESULT_FROM_WIN32(GetLastError());
		}

		// Trigger WH_CALLWNDPROC without waiting for Explorer's UI thread.
		SendNotifyMessage(window, WM_NULL, 0, 0);

		static constexpr DWORD READY_TIMEOUT =
#ifdef _DEBUG
			// do not timeout on debug builds, to allow debugging the DLL while it's loading in explorer
			INFINITE;
#else
			35000;
#endif

		if (!event.wait(READY_TIMEOUT)) [[unlikely]]
		{
			return HRESULT_FROM_WIN32(WAIT_TIMEOUT);
		}
	}

	winrt::com_ptr<IUnknown> service;
	HRESULT hr = GetActiveObject(CLSID_TaskbarAppearanceService, nullptr, service.put());
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
