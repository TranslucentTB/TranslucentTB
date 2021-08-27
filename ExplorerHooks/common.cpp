#include "common.hpp"
#include <handleapi.h>
#include <wil/result.h>

void CloseHandleFailFast(HANDLE handle) noexcept
{
	FAIL_FAST_IF_WIN32_BOOL_FALSE(CloseHandle(handle));
}
