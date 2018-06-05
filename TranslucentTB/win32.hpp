#pragma once
#include "arch.h"
#include <cstdint>
#include <string>
#include <thread>
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
	// Gets location of current module, fatally dies if failed.
	static const std::wstring &GetExeLocation();

	// Checks Windows build number.
	static bool IsAtLeastBuild(const uint32_t &buildNumber);

	// Checks for uniqueness.
	static bool IsSingleInstance();

	// Checks if path exists and is a folder.
	static bool IsDirectory(const std::wstring &directory);

	// Checks if a file exists.
	static bool FileExists(const std::wstring &file);

	// Copies text to the clipboard.
	static void CopyToClipboard(const std::wstring &text);

	// Opens a file in the default text editor.
	static void EditFile(const std::wstring &file);

	// Opens a link in the default browser.
	// NOTE: doesn't attempts to validate the link, make sure it's correct.
	static void OpenLink(const std::wstring &link);

	// Opens a color picker.
	// NOTE: use .join() to wait for input, because this doesn't blocks by default.
	static std::thread PickColor(uint32_t &color);

	// Checks if the start menu is open using a COM interface.
	static bool IsStartVisible();

	// Applies various settings that make code execution more secure.
	static void HardenProcess();

};