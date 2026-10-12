#pragma once
#include <unknwn.h>
#include <winrt/base.h>

// CrossILMarshal
//
// TranslucentTB normally finds the injected appearance service through the Running
// Object Table (RegisterActiveObject / GetActiveObject). The ROT is partitioned by
// process integrity level: if TranslucentTB.exe runs elevated (High IL) while Explorer
// runs at Medium IL, the entry is invisible and GetActiveObject fails with 0x800401E3
// (MK_E_UNAVAILABLE), even though the service registered successfully.
//
// Kernel objects (in Local\) are NOT partitioned by integrity level, so we publish a
// marshalled interface reference in a file mapping as a fallback channel. Client access
// from a higher IL down to a lower IL object is permitted by the default mandatory
// policy, so this works in both directions of elevation mismatch within the same session.
namespace CrossILMarshal
{
	// Server side (the copy of this DLL loaded inside Explorer).
	// Publishes a marshalled reference to `service`, tagged with the Explorer process id.
	HRESULT Publish(::IUnknown* service, DWORD ownerProcessId) noexcept;

	// Client side (the copy of this DLL loaded inside TranslucentTB.exe).
	// Returns the service if a live reference for `expectedProcessId` is available.
	winrt::com_ptr<::IUnknown> Retrieve(DWORD expectedProcessId) noexcept;
}
