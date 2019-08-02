#pragma once
#include "arch.h"
#include <cstdint>
#include <libloaderapi.h>
#include <string_view>
#include <windef.h>
#include <winnt.h>

namespace Localization {
	static constexpr wchar_t FAILED_LOADING_RESOURCE[] = L"[error occured while loading localized string]";
	std::wstring_view LoadLocalizedString(uint16_t resource, WORD lang = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), HINSTANCE hInst = GetModuleHandle(NULL));
}
