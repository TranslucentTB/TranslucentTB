#include "win32.hpp"
#include "arch.h"
#include <fmt/format.h>
#include <optional>
#include <PathCch.h>
#include <processthreadsapi.h>
#include <shellapi.h>
#include <ShlObj.h>
#include <synchapi.h>
#include <thread>
#include <wil/resource.h>
#include <WinBase.h>
#include <winerror.h>
#include <winnt.h>

#include "constants.hpp"
#include "log/ttberror.hpp"
#include "window.hpp"
#include "uwp/uwp.hpp"

std::filesystem::path win32::m_ExeLocation;

std::pair<std::unique_ptr<std::byte[]>, HRESULT> win32::LoadFileVersionInfo(const std::filesystem::path& file, DWORD flags)
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

std::pair<std::filesystem::path, HRESULT> win32::GetProcessFileName(HANDLE process)
{
	DWORD exeLocation_size = LONG_PATH;
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

const std::filesystem::path &win32::GetExeLocation()
{
	if (m_ExeLocation.empty())
	{
		const auto [loc, hr] = GetProcessFileName(GetCurrentProcess());

		if (SUCCEEDED(hr))
		{
			m_ExeLocation = std::move(loc);
		}
		else
		{
			HresultHandle(hr, spdlog::level::critical, L"Failed to determine executable location!");
		}
	}

	return m_ExeLocation;
}

bool win32::IsAtLeastBuild(uint32_t buildNumber)
{
	OSVERSIONINFOEX versionInfo = { sizeof(versionInfo), 10, 0, buildNumber };

	DWORDLONG mask = 0;
	VER_SET_CONDITION(mask, VER_MAJORVERSION, VER_GREATER_EQUAL);
	VER_SET_CONDITION(mask, VER_MINORVERSION, VER_GREATER_EQUAL);
	VER_SET_CONDITION(mask, VER_BUILDNUMBER, VER_GREATER_EQUAL);

	if (VerifyVersionInfo(&versionInfo, VER_MAJORVERSION | VER_MINORVERSION | VER_BUILDNUMBER, mask))
	{
		return true;
	}
	else
	{
		const DWORD error = GetLastError();
		if (error != ERROR_OLD_WIN_VERSION)
		{
			HresultHandle(HRESULT_FROM_WIN32(error), spdlog::level::warn, L"Error obtaining version info.");
		}

		return false;
	}
}

void win32::EditFile(const std::filesystem::path &file)
{
	SHELLEXECUTEINFO info = {
		.cbSize = sizeof(info),
		.fMask = SEE_MASK_CLASSNAME,
		.lpVerb = L"open",
		.lpFile = file.c_str(),
		.nShow = SW_SHOW,
		.lpClass = L"txtfile"
	};

	if (!ShellExecuteEx(&info))
	{
		const DWORD err = GetLastError();
		std::thread([file, err]
		{
			const std::wstring msg =
				fmt::format(fmt(ERROR_MESSAGE L"\n\nFailed to open file \"{}\".\n\n{}\n\nCopy the file location to the clipboard?"), file.native(), Error::MessageFromHRESULT(HRESULT_FROM_WIN32(err)));

			if (MessageBox(Window::NullWindow, msg.c_str(), ERROR_TITLE, MB_ICONWARNING | MB_YESNO | MB_SETFOREGROUND) == IDYES)
			{
				try
				{
					UWP::CopyToClipboard(file.native());
				}
				HresultErrorCatch(spdlog::level::err, L"Failed to copy file path.")
			}
		}).detach();
	}
}

void win32::OpenLink(const std::wstring &link)
{
	SHELLEXECUTEINFO info = {
		.cbSize = sizeof(info),
		.fMask = SEE_MASK_CLASSNAME,
		.lpVerb = L"open",
		.lpFile = link.c_str(),
		.nShow = SW_SHOW,
		.lpClass = L"https" // http causes the file to be downloaded then opened, https does not
	};

	if (!ShellExecuteEx(&info))
	{
		const DWORD err = GetLastError();
		std::thread([link, err]
		{
			const std::wstring msg =
				fmt::format(fmt(ERROR_MESSAGE L"\n\nFailed to open URL \"{}\".\n\n{}\n\nCopy the URL to the clipboard?"), link, Error::MessageFromHRESULT(HRESULT_FROM_WIN32(err)));

			if (MessageBox(Window::NullWindow, msg.c_str(), ERROR_TITLE, MB_ICONWARNING | MB_YESNO | MB_SETFOREGROUND) == IDYES)
			{
				try
				{
					UWP::CopyToClipboard(link);
				}
				HresultErrorCatch(spdlog::level::err, L"Failed to copy URL.")
			}
		}).detach();
	}
}

void win32::HardenProcess()
{
	PROCESS_MITIGATION_ASLR_POLICY aslr_policy;
	if (GetProcessMitigationPolicy(GetCurrentProcess(), ProcessASLRPolicy, &aslr_policy, sizeof(aslr_policy)))
	{
		aslr_policy.EnableForceRelocateImages = true;
		aslr_policy.DisallowStrippedImages = true;
		if (!SetProcessMitigationPolicy(ProcessASLRPolicy, &aslr_policy, sizeof(aslr_policy)))
		{
			LastErrorHandle(spdlog::level::info, L"Couldn't disallow stripped images.");
		}
	}
	else
	{
		LastErrorHandle(spdlog::level::info, L"Couldn't get current ASLR policy.");
	}

#ifdef _CONTROL_FLOW_GUARD
	PROCESS_MITIGATION_CONTROL_FLOW_GUARD_POLICY cfg_policy;
	if (GetProcessMitigationPolicy(GetCurrentProcess(), ProcessControlFlowGuardPolicy, &cfg_policy, sizeof(cfg_policy)))
	{
		cfg_policy.StrictMode = true;
		if (!SetProcessMitigationPolicy(ProcessControlFlowGuardPolicy, &cfg_policy, sizeof(cfg_policy)))
		{
			LastErrorHandle(spdlog::level::info, L"Couldn't enable strict Control Flow Guard.");
		}
	}
	else
	{
		LastErrorHandle(spdlog::level::info, L"Couldn't get current Control Flow Guard policy.");
	}
#endif

	PROCESS_MITIGATION_STRICT_HANDLE_CHECK_POLICY handle_policy {};
	handle_policy.RaiseExceptionOnInvalidHandleReference = true;
	handle_policy.HandleExceptionsPermanentlyEnabled = true;
	if (!SetProcessMitigationPolicy(ProcessStrictHandleCheckPolicy, &handle_policy, sizeof(handle_policy)))
	{
		LastErrorHandle(spdlog::level::info, L"Couldn't enable strict handle checks.");
	}

	PROCESS_MITIGATION_EXTENSION_POINT_DISABLE_POLICY extension_policy {};
	extension_policy.DisableExtensionPoints = true;
	if (!SetProcessMitigationPolicy(ProcessExtensionPointDisablePolicy, &extension_policy, sizeof(extension_policy)))
	{
		LastErrorHandle(spdlog::level::info, L"Couldn't disable extension point DLLs.");
	}

	PROCESS_MITIGATION_BINARY_SIGNATURE_POLICY signature_policy {};
	signature_policy.MitigationOptIn = true;
	if (!SetProcessMitigationPolicy(ProcessSignaturePolicy, &signature_policy, sizeof(signature_policy)))
	{
		LastErrorHandle(spdlog::level::info, L"Couldn't enable image signature enforcement.");
	}


	PROCESS_MITIGATION_IMAGE_LOAD_POLICY load_policy {};
	load_policy.NoLowMandatoryLabelImages = true;
	load_policy.PreferSystem32Images = true;

	// https://devblogs.microsoft.com/oldnewthing/?p=93556
	std::vector<wchar_t> volumePath(LONG_PATH);
	if (GetVolumePathName(GetExeLocation().c_str(), volumePath.data(), LONG_PATH))
	{
		load_policy.NoRemoteImages = GetDriveType(volumePath.data()) != DRIVE_REMOTE;
	}
	else
	{
		LastErrorHandle(spdlog::level::info, L"Unable to get volume path name.");
	}

	if (!SetProcessMitigationPolicy(ProcessImageLoadPolicy, &load_policy, sizeof(load_policy)))
	{
		LastErrorHandle(spdlog::level::info, L"Couldn't set image load policy.");
	}
}

std::pair<std::wstring, HRESULT> win32::GetWindowsBuild()
{
	// Microsoft recommends this themselves
	// https://docs.microsoft.com/en-us/windows/desktop/SysInfo/getting-the-system-version
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

std::pair<Version, HRESULT> win32::GetFixedFileVersion(const std::filesystem::path &file)
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

std::wstring_view win32::GetProcessorArchitecture() noexcept
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

void win32::RevealFile(const std::filesystem::path &file)
{
	wil::unique_cotaskmem_ptr<ITEMIDLIST_ABSOLUTE> list(ILCreateFromPath(file.c_str()));

	if (list)
	{
		const HRESULT hr = SHOpenFolderAndSelectItems(list.get(), 0, nullptr, 0);
		if (FAILED(hr))
		{
			std::thread([file, hr]
			{
				const std::wstring msg =
					fmt::format(fmt(ERROR_MESSAGE L"\n\nFailed to reveal file \"{}\".\n\n{}\n\nCopy the file location to the clipboard?"), file.native(), Error::MessageFromHRESULT(hr));

				if (MessageBox(Window::NullWindow, msg.c_str(), ERROR_TITLE, MB_ICONWARNING | MB_YESNO | MB_SETFOREGROUND) == IDYES)
				{
					try
					{
						UWP::CopyToClipboard(file.native());
					}
					HresultErrorCatch(spdlog::level::err, L"Failed to copy file path.")
				}
			}).detach();
		}
	}
}
