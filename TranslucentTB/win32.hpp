#pragma once
#include "arch.h"
#include <cstdint>
#include <string>
#include <windef.h>

#include "swcadata.hpp"

class user32 {

private:
	typedef bool(WINAPI *pSetWindowCompositionAttribute)(HWND, swca::WINCOMPATTRDATA *);

public:
	static const pSetWindowCompositionAttribute SetWindowCompositionAttribute;

};

class win32 {

private:
	static std::wstring m_ExeLocation;

public:
	static const std::wstring &GetExeLocation();
	static bool IsAtLeastBuild(const uint32_t &buildNumber);
	static bool IsSingleInstance();
	static bool IsDirectory(const std::wstring &directory);
	static bool FileExists(const std::wstring &file);

};