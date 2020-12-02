#pragma once
#include "../arch.h"
#include <intrin.h>
#include <winnt.h>

namespace Util {
	[[noreturn]] inline void QuickAbort() noexcept
	{
		__fastfail(FAST_FAIL_FATAL_APP_EXIT);
	}
}
