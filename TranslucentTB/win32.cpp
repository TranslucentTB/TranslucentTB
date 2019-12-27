#include "win32.hpp"
#include <processthreadsapi.h>

#include "../ProgramLog/error/win32.hpp"

std::filesystem::path win32::s_ExeLocation;

const std::filesystem::path &win32::GetExeLocation()
{
	if (s_ExeLocation.empty())
	{
		auto [loc, hr] = GetProcessFileName(GetCurrentProcess());

		if (SUCCEEDED(hr))
		{
			s_ExeLocation = std::move(loc);
		}
		else
		{
			HresultHandle(hr, spdlog::level::critical, L"Failed to determine executable location!");
		}
	}

	return s_ExeLocation;
}

void win32::HardenProcess()
{
	// Higher logging levels might end up loading more DLLs while we're trying
	// to enable mitigations for DLL loading.
	// OK for debug builds because it makes it readable from the log file
	// but in release we'd rather not.
	static constexpr spdlog::level::level_enum level =
#ifdef _DEBUG
		spdlog::level::warn
#else
		spdlog::level::trace
#endif
		;

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

	PROCESS_MITIGATION_EXTENSION_POINT_DISABLE_POLICY extension_policy {};
	extension_policy.DisableExtensionPoints = true;
	if (!SetProcessMitigationPolicy(ProcessExtensionPointDisablePolicy, &extension_policy, sizeof(extension_policy)))
	{
		LastErrorHandle(level, L"Couldn't disable extension point DLLs.");
	}

	PROCESS_MITIGATION_BINARY_SIGNATURE_POLICY signature_policy {};
	signature_policy.MitigationOptIn = true;
	if (!SetProcessMitigationPolicy(ProcessSignaturePolicy, &signature_policy, sizeof(signature_policy)))
	{
		LastErrorHandle(level, L"Couldn't enable image signature enforcement.");
	}

	PROCESS_MITIGATION_IMAGE_LOAD_POLICY load_policy {};
	load_policy.PreferSystem32Images = true;
	if (!SetProcessMitigationPolicy(ProcessImageLoadPolicy, &load_policy, sizeof(load_policy)))
	{
		LastErrorHandle(level, L"Couldn't set image load policy.");
	}
}
