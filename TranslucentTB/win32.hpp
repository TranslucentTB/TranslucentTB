#pragma once
#include "arch.h"
#include <cstdint>
#include <mutex>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <windef.h>
#include <winrt/base.h>

#include "swcadata.hpp"

namespace user32 {
	extern const swca::pSetWindowCompositionAttribute SetWindowCompositionAttribute;
};

class win32 {
private:
	static std::mutex m_LocationLock;
	static std::wstring m_ExeLocation;
	static std::mutex m_PickerThreadsLock;
	static std::unordered_set<DWORD> m_PickerThreads;

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
	static bool CopyToClipboard(std::wstring_view text);

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

	// Converts a ASCII string to a wide character string.
	static std::wstring CharToWchar(const char *const str);

	// Gets the current Windows build identifier.
	static std::pair<std::wstring, HRESULT> GetWindowsBuild();

	// Gets the FileVersion of a PE binary.
	static std::pair<std::wstring, HRESULT> GetFileVersion(const std::wstring &file);

	// Converts a Windows-style filetime to a unix epoch,
	static unsigned long long FiletimeToUnixEpoch(const FILETIME &time);

	// Gets the current processor architecture as a string.
	static std::wstring GetProcessorArchitecture();
};