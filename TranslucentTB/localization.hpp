#pragma once
#include "arch.h"
#include <cstdint>
#include <windef.h>

#include "util/null_terminated_string_view.hpp"

namespace Localization {
	static constexpr Util::null_terminated_wstring_view FAILED_LOADING = L"[error occurred while loading localized string]";
	Util::null_terminated_wstring_view LoadLocalizedResourceString(uint16_t resource, HINSTANCE hInst, WORD lang = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL));
}
