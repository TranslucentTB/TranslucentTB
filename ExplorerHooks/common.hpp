#pragma once
#include "arch.h"
#include <wil/resource.h>
#include <windef.h>

#include "wilx.hpp"

void CloseHandleFailFast(HANDLE handle) noexcept;
using unique_handle_failfast = wilx::unique_any<CloseHandleFailFast>;
