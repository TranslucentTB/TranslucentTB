#pragma once
#include "arch.h"
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <windef.h>
#include <wingdi.h>

#include "util/numbers.hpp"

class win32 {
private:
	static std::filesystem::path m_ExeLocation;

public:
	// Gets location of the file of a process
	static std::pair<std::filesystem::path, HRESULT> GetProcessFileName(HANDLE process);

	// Gets location of current module, fatally dies if failed.
	static const std::filesystem::path &GetExeLocation();

	// Checks Windows build number.
	static bool IsAtLeastBuild(uint32_t buildNumber);

	// Opens a file in the default text editor.
	static void EditFile(const std::filesystem::path &file);

	// Opens a link in the default browser.
	// NOTE: doesn't attempts to validate the link, make sure it's correct.
	static void OpenLink(const std::wstring &link);

	// Applies various settings that make code execution more secure.
	static void HardenProcess();

	// Gets the current Windows build identifier.
	static std::pair<std::wstring, HRESULT> GetWindowsBuild();

	// Gets the FileVersion of a PE binary.
	static std::pair<std::wstring, HRESULT> GetFileVersion(const std::filesystem::path &file);

	// Gets the current processor architecture as a string.
	static std::wstring_view GetProcessorArchitecture() noexcept;

	// Opens a folder and highlights a file in the File Explorer.
	static void RevealFile(const std::filesystem::path &file);
};