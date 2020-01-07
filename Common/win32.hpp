#pragma once
#include "arch.h"
#include <windef.h>
#include <cstddef>
#include <cstdint>
#include <errhandlingapi.h>
#include <filesystem>
#include <fmt/format.h>
#include <memory>
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

class win32
{
private:
	inline static std::pair<std::unique_ptr<std::byte[]>, HRESULT> LoadFileVersionInfo(const std::filesystem::path &file, DWORD flags = 0)
	{
		const DWORD size = GetFileVersionInfoSizeEx(flags, file.c_str(), nullptr);
		if (!size)
		{
			return { nullptr, HRESULT_FROM_WIN32(GetLastError()) };
		}

		auto data = std::make_unique<std::byte[]>(size);
		if (!GetFileVersionInfoEx(flags, file.c_str(), 0, size, data.get()))
		{
			return { nullptr, HRESULT_FROM_WIN32(GetLastError()) };
		}

		return { std::move(data), S_OK };
	}

	inline static HRESULT ShellExec(SHELLEXECUTEINFO &info) noexcept
	{
		if (ShellExecuteEx(&info))
		{
			return S_OK;
		}
		else
		{
			return HRESULT_FROM_WIN32(GetLastError());
		}
	}

public:
	// Gets location of the file of a process
	inline static std::pair<std::filesystem::path, HRESULT> GetProcessFileName(HANDLE process)
	{
		DWORD exeLocation_size = wil::max_extended_path_length + 1;

		std::wstring exeLocation;
		exeLocation.resize(exeLocation_size);
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

	inline static std::pair<std::filesystem::path, HRESULT> GetModulePath(HMODULE mod)
	{
		const DWORD size = GetModuleFileName(mod, nullptr, 0);
		if (!size)
		{
			return { { }, HRESULT_FROM_WIN32(GetLastError()) };
		}

		std::wstring location;
		location.reserve(size);
		if (const DWORD used = GetModuleFileName(mod, location.data(), size))
		{
			location.resize(used);
			return { std::move(location), S_OK };
		}
		else
		{
			return { { }, HRESULT_FROM_WIN32(GetLastError()) };
		}
	}

	// Checks Windows build number.
	inline static bool IsAtLeastBuild(uint32_t buildNumber) noexcept
	{
		OSVERSIONINFOEX versionInfo = { sizeof(versionInfo), 10, 0, buildNumber };

		DWORDLONG mask = 0;
		VER_SET_CONDITION(mask, VER_MAJORVERSION, VER_GREATER_EQUAL);
		VER_SET_CONDITION(mask, VER_MINORVERSION, VER_GREATER_EQUAL);
		VER_SET_CONDITION(mask, VER_BUILDNUMBER, VER_GREATER_EQUAL);

		return VerifyVersionInfo(&versionInfo, VER_MAJORVERSION | VER_MINORVERSION | VER_BUILDNUMBER, mask);
	}

	// Opens a file in the default text editor.
	inline static HRESULT EditFile(const std::filesystem::path &file) noexcept
	{
		SHELLEXECUTEINFO info = {
			.cbSize = sizeof(info),
			.fMask = SEE_MASK_CLASSNAME,
			.lpVerb = L"open",
			.lpFile = file.c_str(),
			.nShow = SW_SHOW,
			.lpClass = L".txt"
		};

		return ShellExec(info);
	}

	// Opens a link in the default browser.
	// NOTE: doesn't attempts to validate the link, make sure it's correct.
	inline static HRESULT OpenLink(Util::null_terminated_wstring_view link) noexcept
	{
		SHELLEXECUTEINFO info = {
			.cbSize = sizeof(info),
			.fMask = SEE_MASK_CLASSNAME,
			.lpVerb = L"open",
			.lpFile = link.c_str(),
			.nShow = SW_SHOW,
			.lpClass = L"https" // http causes the file to be downloaded then opened, https does not
		};

		return ShellExec(info);
	}

	// Gets the current Windows build identifier.
	inline static std::pair<std::wstring, HRESULT> GetWindowsBuild()
	{
		// Microsoft recommends this themselves
		// https://docs.microsoft.com/windows/desktop/SysInfo/getting-the-system-version
		wil::unique_cotaskmem_string system32;
		const HRESULT hr = SHGetKnownFolderPath(FOLDERID_System, KF_FLAG_DEFAULT, nullptr, system32.put());
		if (FAILED(hr))
		{
			return { { }, hr };
		}

		std::filesystem::path user32 = system32.get();
		user32 /= L"user32.dll";

		const auto [version, hr2] = GetFixedFileVersion(user32);
		if (SUCCEEDED(hr2))
		{
			return { version.ToString(), S_OK };
		}
		else
		{
			return { { }, hr2 };
		}
	}

	// Gets the language-neutral FileVersion of a PE binary.
	inline static std::pair<Version, HRESULT> GetFixedFileVersion(const std::filesystem::path &file)
	{
		const auto [data, hr] = LoadFileVersionInfo(file, FILE_VER_GET_NEUTRAL);
		if (FAILED(hr))
		{
			return { { }, hr };
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

	// Opens a folder and highlights a file in the File Explorer.
	inline static HRESULT RevealFile(const std::filesystem::path &file) noexcept
	{
		wil::unique_cotaskmem_ptr<ITEMIDLIST_ABSOLUTE> list(ILCreateFromPath(file.c_str()));

		return SHOpenFolderAndSelectItems(list.get(), 0, nullptr, 0);
	}

	static constexpr bool RectFitsInRect(const RECT &outer, const RECT &inner)
	{
		return inner.right <= outer.right && inner.left >= outer.left &&
			outer.top <= inner.top && outer.bottom >= inner.bottom;
	}

	struct FilenameEqual {
		using is_transparent = void;
		inline bool operator()(std::wstring_view l, std::wstring_view r) const
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
					// it wasn't, call an API and break out
					SlowHash(hash, k.substr(i));
					break;
				}
			}

			return hash;
		}

	private:
		inline static void SlowHash(std::size_t &hash, std::wstring_view k)
		{
			fmt::wmemory_buffer buf;
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

				for (const wchar_t c : buf)
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

#ifdef _TRANSLUCENTTB_EXE
private:
	static std::filesystem::path s_ExeLocation;

public:
	// Gets location of current module, fatally dies if failed.
	static const std::filesystem::path &GetExeLocation();

	// Applies various settings that make code execution more secure.
	static void HardenProcess();
#endif
};
