#include "arch.h"
#include <errhandlingapi.h>
#include <heapapi.h>
#include <processthreadsapi.h>
#include <synchapi.h>
#include <wil/resource.h>
#include "winrt.hpp"

#include "application.hpp"
#include "constants.hpp"
#include "mainappwindow.hpp"
#include "../ProgramLog/error/win32.hpp"
#include "../ProgramLog/error/winrt.hpp"
#include "../ProgramLog/log.hpp"
#include "uwp/uwp.hpp"

void HardenProcess()
{
	// Higher logging levels might end up loading more DLLs while we're trying
	// to enable mitigations for DLL loading.
	// OK for debug builds because it makes it readable from the log file
	// but in release we'd rather not.
	// This entire thing happens before config is loaded, so trace will never log.
	static constexpr spdlog::level::level_enum level =
#ifdef _DEBUG
		spdlog::level::warn;
#else
		spdlog::level::trace;
#endif

	if (!SetSearchPathMode(BASE_SEARCH_PATH_ENABLE_SAFE_SEARCHMODE | BASE_SEARCH_PATH_PERMANENT))
	{
		LastErrorHandle(level, L"Couldn't enable safe DLL search mode.");
	}

	if (!HeapSetInformation(nullptr, HeapEnableTerminationOnCorruption, nullptr, 0))
	{
		LastErrorHandle(level, L"Couldn't enable termination on heap corruption.");
	}

	PROCESS_MITIGATION_STRICT_HANDLE_CHECK_POLICY handle_policy{};
	handle_policy.RaiseExceptionOnInvalidHandleReference = true;
	handle_policy.HandleExceptionsPermanentlyEnabled = true;
	if (!SetProcessMitigationPolicy(ProcessStrictHandleCheckPolicy, &handle_policy, sizeof(handle_policy)))
	{
		LastErrorHandle(level, L"Couldn't enable strict handle checks.");
	}

	PROCESS_MITIGATION_ASLR_POLICY aslr_policy;
	if (GetProcessMitigationPolicy(GetCurrentProcess(), ProcessASLRPolicy, &aslr_policy, sizeof(aslr_policy)))
	{
		aslr_policy.EnableForceRelocateImages = true;
		aslr_policy.DisallowStrippedImages = true;
		if (!SetProcessMitigationPolicy(ProcessASLRPolicy, &aslr_policy, sizeof(aslr_policy)))
		{
			LastErrorHandle(level, L"Couldn't enable image force relocation.");
		}
	}
	else
	{
		LastErrorHandle(level, L"Couldn't get current ASLR policy.");
	}

#ifdef _CONTROL_FLOW_GUARD
	PROCESS_MITIGATION_CONTROL_FLOW_GUARD_POLICY cfg_policy;
	if (GetProcessMitigationPolicy(GetCurrentProcess(), ProcessControlFlowGuardPolicy, &cfg_policy, sizeof(cfg_policy)))
	{
		cfg_policy.StrictMode = true;
		if (!SetProcessMitigationPolicy(ProcessControlFlowGuardPolicy, &cfg_policy, sizeof(cfg_policy)))
		{
			LastErrorHandle(level, L"Couldn't enable strict Control Flow Guard.");
		}
	}
	else
	{
		LastErrorHandle(level, L"Couldn't get current Control Flow Guard policy.");
	}
#endif

	PROCESS_MITIGATION_EXTENSION_POINT_DISABLE_POLICY extension_policy{};
	extension_policy.DisableExtensionPoints = true;
	if (!SetProcessMitigationPolicy(ProcessExtensionPointDisablePolicy, &extension_policy, sizeof(extension_policy)))
	{
		LastErrorHandle(level, L"Couldn't disable extension point DLLs.");
	}

	PROCESS_MITIGATION_BINARY_SIGNATURE_POLICY signature_policy{};
	signature_policy.MitigationOptIn = true;
	if (!SetProcessMitigationPolicy(ProcessSignaturePolicy, &signature_policy, sizeof(signature_policy)))
	{
		LastErrorHandle(level, L"Couldn't enable image signature enforcement.");
	}

	PROCESS_MITIGATION_IMAGE_LOAD_POLICY load_policy{};
	load_policy.PreferSystem32Images = true;
	if (!SetProcessMitigationPolicy(ProcessImageLoadPolicy, &load_policy, sizeof(load_policy)))
	{
		LastErrorHandle(level, L"Couldn't set image load policy.");
	}
}

_Use_decl_annotations_ int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, wchar_t *, int)
{
	auto storageFolder = UWP::GetAppStorageFolder();
	Log::Initialize(storageFolder);
	HardenProcess();

	wil::unique_mutex mutex(MUTEX_GUID.c_str());
	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		// If there already is another instance running, tell it to exit
		MainAppWindow::CloseRemote();
	}

	try
	{
		winrt::init_apartment(winrt::apartment_type::single_threaded);
	}
	HresultErrorCatch(spdlog::level::critical, L"Initialization of Windows Runtime failed.");

	// Run the main program loop. When this method exits, TranslucentTB itself is about to exit.
	const auto ret = Application(hInstance, std::move(storageFolder)).Run();

	// why are we brutally terminating you might ask?
	// Windows.UI.Xaml.dll likes to read null pointers if you exit the app too quickly after
	// closing a XAML window. While this is not a big deal for the user since we
	// are about to exit and saved everything, it pollutes telemetry and system crash data.
	// It's not easily doable to catch SEH exceptions in post-Main DLL unload, so instead just
	// brutally terminating will work.
	// Caused specifically by ColorPicker, go figure: https://github.com/microsoft/microsoft-ui-xaml/issues/3541
	TerminateProcess(GetCurrentProcess(), ret);

	// just in case
	return ret;
}
