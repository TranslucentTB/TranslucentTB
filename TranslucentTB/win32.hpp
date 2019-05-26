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

	// Copies text to the clipboard.
	static bool CopyToClipboard(std::wstring_view text);

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

	// Converts a Windows-style filetime to a unix epoch,
	inline static uint64_t FiletimeToUnixEpoch(FILETIME time) noexcept
	{
		auto timeStamp = Util::WordCast<uint64_t>(time);

		// FILETIME is in hundreds of nanoseconds, but Unix timestamps are in seconds.
		timeStamp /= 10000000;

		// Unix timestamps are since 1970, but FILETIME is since 1601.
		// Black magic told me there are 11644473600 seconds between the two years.
		timeStamp -= 11644473600;

		return timeStamp;
	}

	// Gets the current processor architecture as a string.
	static std::wstring_view GetProcessorArchitecture() noexcept;

	// Opens a folder and highlights a file in the File Explorer.
	static void RevealFile(const std::filesystem::path &file);

	static constexpr uint32_t SwapColorOrder(uint32_t color)
	{
		const uint8_t r = GetBValue(color);
		const uint8_t g = GetGValue(color);
		const uint8_t b = GetRValue(color);

		return RGB(r, g, b);
	}
};