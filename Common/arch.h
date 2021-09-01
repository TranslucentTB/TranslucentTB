#pragma once

// Those are defines required by various Windows headers to build.

#if defined(_M_AMD64)
# ifndef _AMD64_
#  define _AMD64_
# endif
#elif defined(_M_ARM64)
# ifndef _ARM64_
#  define _ARM64_
# endif
#else
# error "Target architecture not recognized"
#endif
