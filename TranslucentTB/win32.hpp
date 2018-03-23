#pragma once
#include "arch.h"
#include <cstdint>
#include <string>
#include <windef.h>
#include <WinBase.h>

#include "swcadata.hpp"

namespace user32 {

	typedef bool(WINAPI *pSetWindowCompositionAttribute)(HWND, swca::WINCOMPATTRDATA *);
	static const pSetWindowCompositionAttribute SetWindowCompositionAttribute =
		reinterpret_cast<pSetWindowCompositionAttribute>(
			GetProcAddress(GetModuleHandle(L"user32.dll"), "SetWindowCompositionAttribute")
		);

};

class win32 {

public:
	static bool IsAtLeastBuild(const uint32_t &buildNumber);
	static bool IsSingleInstance();
	static bool IsDirectory(const std::wstring &directory);
	static bool FileExists(const std::wstring &file);

};