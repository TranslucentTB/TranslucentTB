#pragma once
#include "arch.h"
#include <wil/resource.h>
#include <windef.h>

void CloseHandleFailFast(HANDLE handle) noexcept;
using unique_handle_failfast = wil::unique_any<HANDLE, decltype(&CloseHandleFailFast), CloseHandleFailFast>;
