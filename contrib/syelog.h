//////////////////////////////////////////////////////////////////////////////
//
//  Detours Test Program (syelog.h of syelog.lib)
//
//  Microsoft Research Detours Package
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
#pragma once
#ifndef _SYELOGD_H_
#define _SYELOGD_H_
#include <stdarg.h>

#pragma pack(push, 1)
#pragma warning(push)
#pragma warning(disable: 4200)

//////////////////////////////////////////////////////////////////////////////
//
//
#define SYELOG_PIPE_NAMEA       "\\\\.\\pipe\\syelog"
#define SYELOG_PIPE_NAMEW       L"\\\\.\\pipe\\syelog"
#ifdef UNICODE
#define SYELOG_PIPE_NAME        SYELOG_PIPE_NAMEW
#else
#define SYELOG_PIPE_NAME        SYELOG_PIPE_NAMEA
#endif

//////////////////////////////////////////////////////////////////////////////
//
#define SYELOG_MAXIMUM_MESSAGE  4086    // 4096 - sizeof(header stuff)

typedef struct _SYELOG_MESSAGE
{
    USHORT      nBytes;
    BYTE        nFacility;
    BYTE        nSeverity;
    DWORD       nProcessId;
    FILETIME    ftOccurance;
    BOOL        fTerminate;
    CHAR        szMessage[SYELOG_MAXIMUM_MESSAGE];
} SYELOG_MESSAGE, *PSYELOG_MESSAGE;


// Facility Codes.
//
#define SYELOG_FACILITY_KERNEL          0x10            // OS Kernel
#define SYELOG_FACILITY_SECURITY        0x20            // OS Security
#define SYELOG_FACILITY_LOGGING         0x30            // OS Logging-internal
#define SYELOG_FACILITY_SERVICE         0x40            // User-mode system daemon
#define SYELOG_FACILITY_APPLICATION     0x50            // User-mode application
#define SYELOG_FACILITY_USER            0x60            // User self-generated.
#define SYELOG_FACILITY_LOCAL0          0x70            // Locally defined.
#define SYELOG_FACILITY_LOCAL1          0x71            // Locally defined.
#define SYELOG_FACILITY_LOCAL2          0x72            // Locally defined.
#define SYELOG_FACILITY_LOCAL3          0x73            // Locally defined.
#define SYELOG_FACILITY_LOCAL4          0x74            // Locally defined.
#define SYELOG_FACILITY_LOCAL5          0x75            // Locally defined.
#define SYELOG_FACILITY_LOCAL6          0x76            // Locally defined.
#define SYELOG_FACILITY_LOCAL7          0x77            // Locally defined.
#define SYELOG_FACILITY_LOCAL8          0x78            // Locally defined.
#define SYELOG_FACILITY_LOCAL9          0x79            // Locally defined.

// Severity Codes.
//
#define SYELOG_SEVERITY_FATAL           0x00            // System is dead.
#define SYELOG_SEVERITY_ALERT           0x10            // Take action immediately.
#define SYELOG_SEVERITY_CRITICAL        0x20            // Critical condition.
#define SYELOG_SEVERITY_ERROR           0x30            // Error
#define SYELOG_SEVERITY_WARNING         0x40            // Warning
#define SYELOG_SEVERITY_NOTICE          0x50            // Significant condition.
#define SYELOG_SEVERITY_INFORMATION     0x60            // Informational
#define SYELOG_SEVERITY_AUDIT_FAIL      0x66            // Audit Failed
#define SYELOG_SEVERITY_AUDIT_PASS      0x67            // Audit Succeeeded
#define SYELOG_SEVERITY_DEBUG           0x70            // Debugging

// Logging Functions.
//
VOID SyelogOpen(PCSTR pszIdentifier, BYTE nFacility);
VOID Syelog(BYTE nSeverity, PCSTR pszMsgf, ...);
VOID SyelogV(BYTE nSeverity, PCSTR pszMsgf, va_list args);
VOID SyelogClose(BOOL fTerminate);

#pragma warning(pop)
#pragma pack(pop)

#endif //  _SYELOGD_H_
//
///////////////////////////////////////////////////////////////// End of File.
