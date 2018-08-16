#pragma once

// Those are defines required by various Windows headers to build.

#if defined(_M_IX86) && !defined(_x86_)
#define _X86_
#endif

#if defined(_M_AMD64) && !defined(_AMD64_)
#define _AMD64_
#endif

#if defined(_M_ARM) && !defined(_ARM_)
#define _ARM_
#endif

#if defined(_M_ARM64) && !defined(_ARM64_)
#define _ARM64_
#endif