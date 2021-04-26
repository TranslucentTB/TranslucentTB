#pragma once
#include "../arch.h"
#include <windef.h>
#include <winternl.h>

// UserDefinedType: _SYSTEM_PROCESS_ID_INFORMATION
// Data           :   this+0x0, Member, Type: void *, ProcessId
// Data           :   this+0x8, Member, Type: struct _UNICODE_STRING, ImageName
struct SYSTEM_PROCESS_ID_INFORMATION {
	PVOID          ProcessId;
	UNICODE_STRING ImageName;
};

// Enum           : _SYSTEM_INFORMATION_CLASS, Type: int
// Data           :   constant 0x0, Constant, Type: int, SystemBasicInformation
// Data           :   constant 0x1, Constant, Type: int, SystemProcessorInformation
// Data           :   constant 0x2, Constant, Type: int, SystemPerformanceInformation
// ...
// Data           :   constant 0x58, Constant, Type: int, SystemProcessIdInformation
// ...
// Data           :   constant 0xE4, Constant, Type: int, MaxSystemInfoClass
enum SYSTEM_INFORMATION_CLASS_UNDOC : INT {
	SystemProcessIdInformation = 0x58
};

// This is normally defined in ntstatus.h but that header conflicts with user mode headers
static constexpr NTSTATUS STATUS_INFO_LENGTH_MISMATCH_UNDOC = 0xC0000004L;

typedef __kernel_entry NTSTATUS (NTAPI* PFN_NT_QUERY_SYSTEM_INFORMATION)(IN SYSTEM_INFORMATION_CLASS SystemInformationClass, OUT PVOID SystemInformation, IN ULONG SystemInformationLength, OUT PULONG ReturnLength OPTIONAL);
