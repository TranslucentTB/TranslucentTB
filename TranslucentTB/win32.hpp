#pragma once
#include "arch.h"
#include <cstdint>
#include <mutex>
#include <string>
#include <type_traits>
#include <vector>
#include <windef.h>
#include <winrt/base.h>

#include "swcadata.hpp"

class user32 {

private:
	using pSetWindowCompositionAttribute = std::add_pointer_t<BOOL WINAPI(HWND, swca::WINCOMPATTRDATA *)>;

public:
	static const pSetWindowCompositionAttribute SetWindowCompositionAttribute;

};

class win32 {

private:
	static std::wstring m_ExeLocation;
	static std::mutex m_PickerThreadsLock;
	static std::vector<DWORD> m_PickerThreads;

	static DWORD WINAPI PickerThreadProc(LPVOID data);
	static BOOL CALLBACK EnumThreadWindowsProc(HWND hwnd, LPARAM lParam);

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
	// NOTE: the function returns the thread ID, use it with OpenThread and
	// WaitForSingleObject if you want to block for input.
	static DWORD PickColor(uint32_t &color);

	// Cancels all active color pickers.
	static void ClosePickers();

	// Applies various settings that make code execution more secure.
	static void HardenProcess();

	// Converts a ASCII string to a wide character string
	static std::wstring CharToWchar(const char *const str);

};