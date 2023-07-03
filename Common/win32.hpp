#pragma once
#include "arch.h"
#include <bit>
#include <windef.h>
#include <cstddef>
#include <cstdint>
#include <errhandlingapi.h>
#include <filesystem>
#include <memory>
#include <libloaderapi.h>
#include <processthreadsapi.h>
#include <winbase.h>
#include <shellapi.h>
#include <Shlobj.h>
#include <string>
#include <string_view>
#include <stringapiset.h>
#include <sysinfoapi.h>
#include <system_error>
#include <unordered_set>
#include <utility>
#include <wil/resource.h>
#include <wil/safecast.h>
#include <wil/win32_helpers.h>
#include <winerror.h>
#include <wingdi.h>
#include <winver.h>

#include "constants.hpp"
#include "util/hash.hpp"
#include "util/null_terminated_string_view.hpp"
#include "util/strings.hpp"
#include "version.hpp"

class win32 {
private:
	static std::unique_ptr<std::byte[]> LoadFileVersionInfo(const std::filesystem::path &file, DWORD flags = 0)
	{
		const DWORD size = GetFileVersionInfoSizeEx(flags, file.c_str(), nullptr);
		if (!size)
		{
			return nullptr;
		}

		auto data = std::make_unique<std::byte[]>(size);
		if (!GetFileVersionInfoEx(flags, file.c_str(), 0, size, data.get()))
		{
			return nullptr;
		}

		return data;
	}

public:
	// Gets location of the file of a process
	inline static std::pair<std::filesystem::path, HRESULT> GetProcessFileName(HANDLE process)
	{
		std::wstring exeLocation;
		exeLocation.resize(wil::max_extended_path_length);

		DWORD exeLocation_size = static_cast<DWORD>(exeLocation.size()) + 1;
		if (QueryFullProcessImageName(process, 0, exeLocation.data(), &exeLocation_size))
		{
			exeLocation.resize(exeLocation_size);
			return { std::move(exeLocation), S_OK };
		}
		else
		{
			return { { }, HRESULT_FROM_WIN32(GetLastError()) };
		}
	}

	// Gets location of current process
	inline static std::pair<std::filesystem::path, HRESULT> GetExeLocation()
	{
		return GetProcessFileName(GetCurrentProcess());
	}

	// Gets the location of a loaded DLL
	inline static std::pair<std::filesystem::path, HRESULT> GetDllLocation(HMODULE hModule)
	{
		std::wstring location;
		location.resize(wil::max_path_length);

		if (auto size = GetModuleFileName(hModule, location.data(), static_cast<DWORD>(location.size()) + 1))
		{
			if (size == location.size() + 1 && GetLastError() == ERROR_INSUFFICIENT_BUFFER) [[unlikely]]
			{
				location.resize(wil::max_extended_path_length);
				size = GetModuleFileName(hModule, location.data(), static_cast<DWORD>(location.size()) + 1);

				if (!size) [[unlikely]]
				{
					return { {}, HRESULT_FROM_WIN32(GetLastError()) };
				}
			}

			location.resize(size);

			return { std::move(location), S_OK };
		}
		else
		{
			return { {}, HRESULT_FROM_WIN32(GetLastError()) };
		}
	}

	// Opens a file in the default text editor.
	inline static HRESULT EditFile(const std::filesystem::path &file) noexcept
	{
		SHELLEXECUTEINFO info = {
			.cbSize = sizeof(info),
			.fMask = SEE_MASK_CLASSNAME | SEE_MASK_FLAG_NO_UI,
			.lpVerb = L"open",
			.lpFile = file.c_str(),
			.nShow = SW_SHOW,
			.lpClass = L".txt"
		};

		if (ShellExecuteEx(&info))
		{
			return S_OK;
		}
		else
		{
			return HRESULT_FROM_WIN32(GetLastError());
		}
	}

	// Gets the current Windows build identifier.
	// Unfortunately there is no good generic way to detect if a KB is installed.
	// Checking via WIM or the WU API only lists KBs installed since the OS
	// was clean installed, so if a KB superseding the one we are looking for
	// has been released, then the OS will directly install that and never list
	// the specific KB we want. We have to check the revision ID for that
	// but VerifyVersionInfo doesn't support checking revision ID, so we have
	// to manually read it off ntoskrnl.exe (which is the lesser evil compared
	// to calling a driver-only API like RtlGetVersion).
	// Prefer feature checking where possible - however sometimes for mundane
	// bug fixes or fixes/changes to undocumented feature that isn't possible.
	inline static std::pair<Version, HRESULT> GetWindowsBuild()
	{
		wil::unique_cotaskmem_string system32;
		const HRESULT hr = SHGetKnownFolderPath(FOLDERID_System, KF_FLAG_DEFAULT, nullptr, system32.put());
		if (FAILED(hr))
		{
			return { { }, hr };
		}

		std::filesystem::path ntoskrnl = system32.get();
		ntoskrnl /= L"ntoskrnl.exe";

		return GetFixedFileVersion(ntoskrnl);
	}

	inline static bool IsAtLeastBuild(uint32_t buildNumber) noexcept
	{
		OSVERSIONINFOEX versionInfo = { sizeof(versionInfo), 10, 0, buildNumber };

		DWORDLONG mask = 0;
		VER_SET_CONDITION(mask, VER_MAJORVERSION, VER_GREATER_EQUAL);
		VER_SET_CONDITION(mask, VER_MINORVERSION, VER_GREATER_EQUAL);
		VER_SET_CONDITION(mask, VER_BUILDNUMBER, VER_GREATER_EQUAL);

		return VerifyVersionInfo(&versionInfo, VER_MAJORVERSION | VER_MINORVERSION | VER_BUILDNUMBER, mask);
	}

	inline static bool IsExactBuild(uint32_t buildNumber) noexcept
	{
		OSVERSIONINFOEX versionInfo = { sizeof(versionInfo), 10, 0, buildNumber };

		DWORDLONG mask = 0;
		VER_SET_CONDITION(mask, VER_MAJORVERSION, VER_EQUAL);
		VER_SET_CONDITION(mask, VER_MINORVERSION, VER_EQUAL);
		VER_SET_CONDITION(mask, VER_BUILDNUMBER, VER_EQUAL);

		return VerifyVersionInfo(&versionInfo, VER_MAJORVERSION | VER_MINORVERSION | VER_BUILDNUMBER, mask);
	}

	// Gets the language-neutral FileVersion of a PE binary.
	inline static std::pair<Version, HRESULT> GetFixedFileVersion(const std::filesystem::path &file)
	{
		const auto data = LoadFileVersionInfo(file, FILE_VER_GET_NEUTRAL);
		if (!data)
		{
			return { { }, HRESULT_FROM_WIN32(GetLastError()) };
		}

		VS_FIXEDFILEINFO *fixedFileInfo;
		unsigned int length;
		if (!VerQueryValue(data.get(), L"\\", reinterpret_cast<void **>(&fixedFileInfo), &length))
		{
			return { { }, HRESULT_FROM_WIN32(GetLastError()) };
		}

		return { Version::FromHighLow(fixedFileInfo->dwProductVersionMS, fixedFileInfo->dwProductVersionLS), S_OK };
	}

	// Gets the current processor architecture as a string.
	inline static Util::null_terminated_wstring_view GetProcessorArchitecture() noexcept
	{
		SYSTEM_INFO info;
		GetNativeSystemInfo(&info);

		switch (info.wProcessorArchitecture)
		{
		case PROCESSOR_ARCHITECTURE_AMD64:
			return L"x64";

		case PROCESSOR_ARCHITECTURE_INTEL:
			return L"x86";

		case PROCESSOR_ARCHITECTURE_ARM64:
			return L"ARM64";

		case PROCESSOR_ARCHITECTURE_ARM:
			return L"ARM";

		case PROCESSOR_ARCHITECTURE_IA64:
			return L"Itanium";

		case PROCESSOR_ARCHITECTURE_UNKNOWN:
			return L"Unknown";

		default:
			return L"Invalid";
		}
	}

	static constexpr bool RectFitsInRect(const RECT &outer, const RECT &inner) noexcept
	{
		return inner.right <= outer.right && inner.left >= outer.left &&
			outer.top <= inner.top && outer.bottom >= inner.bottom;
	}

	static constexpr void OffsetRect(RECT &rect, int x, int y) noexcept
	{
		rect.left += x;
		rect.right += x;
		rect.top += y;
		rect.bottom += y;
	}

	inline static bool IsSameFilename(std::wstring_view l, std::wstring_view r)
	{
		const int result = CompareStringOrdinal(
			l.data(), wil::safe_cast<int>(l.length()),
			r.data(), wil::safe_cast<int>(r.length()),
			true
		);

		if (result)
		{
			return result == CSTR_EQUAL;
		}
		else
		{
			throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), "Failed to compare strings");
		}
	}

	struct FilenameEqual {
		using is_transparent = void;
		inline bool operator()(std::wstring_view l, std::wstring_view r) const
		{
			return IsSameFilename(l, r);
		}
	};

	struct FilenameHash {
		using transparent_key_equal = FilenameEqual;
		inline std::size_t operator()(std::wstring_view k) const
		{
			std::size_t hash = Util::INITIAL_HASH_VALUE;
			for (std::size_t i = 0; i < k.length(); ++i)
			{
				if (Util::IsAscii(k[i]))
				{
					// if the string is all ascii characters, avoid API calls
					// this is much faster due to being branchless code
					Util::HashCharacter(hash, Util::AsciiToUpper(k[i]));
				}
				else
				{
					// when we encounter a non-ascii character, call an API to hash the rest and break out
					SlowHash(hash, k.substr(i));
					break;
				}
			}

			return hash;
		}

	private:
		inline static void SlowHash(std::size_t &hash, std::wstring_view k)
		{
			std::wstring buf;
			buf.resize(k.length());

			const int result = LCMapStringEx(
				LOCALE_NAME_INVARIANT, LCMAP_UPPERCASE,
				k.data(), wil::safe_cast<int>(k.length()),
				buf.data(), wil::safe_cast<int>(buf.size()),
				nullptr, nullptr, 0
			);

			if (result)
			{
				buf.resize(result);

				for (const wchar_t &c : buf)
				{
					Util::HashCharacter(hash, c);
				}
			}
			else
			{
				throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), "Failed to hash string");
			}
		}
	};

	using FilenameSet = std::unordered_set<std::wstring, FilenameHash, FilenameEqual>;

	template<typename T>
	using FilenameMap = std::unordered_map<std::wstring, T, FilenameHash, FilenameEqual>;
};
