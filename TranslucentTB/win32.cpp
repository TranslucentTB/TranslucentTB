#include "win32.hpp"
#include "arch.h"
#include <cstddef>
#include <memory>
#include <optional>
#include <PathCch.h>
#include <processthreadsapi.h>
#include <shellapi.h>
#include <ShlObj.h>
#include <sstream>
#include <synchapi.h>
#include <utility>
#include <wil/resource.h>
#include <WinBase.h>
#include <winerror.h>
#include <winnt.h>

#include "smart/autofree.hpp"
#include "smart/autounlock.hpp"
#include "smart/clipboardcontext.hpp"
#include "constants.hpp"
#include "ttberror.hpp"
#include "ttblog.hpp"
#include "windows/window.hpp"

std::filesystem::path win32::m_ExeLocation;

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
			ErrorHandle(hr, Error::Level::Fatal, L"Failed to determine executable location!");
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
		DWORD error = GetLastError();
		if (error != ERROR_OLD_WIN_VERSION)
		{
			ErrorHandle(HRESULT_FROM_WIN32(error), Error::Level::Log, L"Error obtaining version info.");
		}

		return false;
	}
}

bool win32::CopyToClipboard(std::wstring_view text)
{
	const ClipboardContext context;
	if (!context)
	{
		LastErrorHandle(Error::Level::Error, L"Failed to open clipboard.");
		return false;
	}

	if (!EmptyClipboard())
	{
		LastErrorHandle(Error::Level::Error, L"Failed to empty clipboard.");
		return false;
	}

	auto data = AutoFree::GlobalHandle<wchar_t[]>::Alloc(text.length() + 1);
	if (!data)
	{
		LastErrorHandle(Error::Level::Error, L"Failed to allocate memory for the clipboard.");
		return false;
	}

	{
		AutoUnlock lock(data);
		if (!data)
		{
			LastErrorHandle(Error::Level::Error, L"Failed to lock memory for the clipboard.");
			return false;
		}
		text.copy(data.get(), text.length());
	}

	if (!SetClipboardData(CF_UNICODETEXT, data.detach()))
	{
		LastErrorHandle(Error::Level::Error, L"Failed to copy data to clipboard.");
		return false;
	}

	return true;
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
			std::wostringstream str;
			str
				<< L"Failed to open file \"" << file.native() << L"\".\n\n"
				<< Error::ExceptionFromHRESULT(HRESULT_FROM_WIN32(err))
				<< L"\n\nCopy the file location to the clipboard?";

			if (MessageBox(Window::NullWindow, str.str().c_str(), NAME L" - Error", MB_ICONWARNING | MB_YESNO | MB_SETFOREGROUND) == IDYES)
			{
				CopyToClipboard(file.native());
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
		.lpClass = link[4] == L's' ? L"https" : L"http"
	};

	if (!ShellExecuteEx(&info))
	{
		const DWORD err = GetLastError();
		std::thread([link, err]
		{
			std::wstring boxbuffer =
				L"Failed to open URL \"" + link + L"\"." +
				L"\n\n" + Error::ExceptionFromHRESULT(HRESULT_FROM_WIN32(err)) +
				L"\n\nCopy the URL to the clipboard?";

			if (MessageBox(Window::NullWindow, boxbuffer.c_str(), NAME L" - Error", MB_ICONWARNING | MB_YESNO | MB_SETFOREGROUND) == IDYES)
			{
				CopyToClipboard(link);
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
			LastErrorHandle(Error::Level::Log, L"Couldn't disallow stripped images.");
		}
	}
	else
	{
		LastErrorHandle(Error::Level::Log, L"Couldn't get current ASLR policy.");
	}

#ifdef _CONTROL_FLOW_GUARD
	PROCESS_MITIGATION_CONTROL_FLOW_GUARD_POLICY cfg_policy;
	if (GetProcessMitigationPolicy(GetCurrentProcess(), ProcessControlFlowGuardPolicy, &cfg_policy, sizeof(cfg_policy)))
	{
		cfg_policy.StrictMode = true;
		if (!SetProcessMitigationPolicy(ProcessControlFlowGuardPolicy, &cfg_policy, sizeof(cfg_policy)))
		{
			LastErrorHandle(Error::Level::Log, L"Couldn't enable strict Control Flow Guard.");
		}
	}
	else
	{
		LastErrorHandle(Error::Level::Log, L"Couldn't get current Control Flow Guard policy.");
	}
#endif

	PROCESS_MITIGATION_DYNAMIC_CODE_POLICY code_policy {};
	code_policy.ProhibitDynamicCode = true;
	code_policy.AllowThreadOptOut = false;
	code_policy.AllowRemoteDowngrade = false;
	if (!SetProcessMitigationPolicy(ProcessDynamicCodePolicy, &code_policy, sizeof(code_policy)))
	{
		LastErrorHandle(Error::Level::Log, L"Couldn't disable dynamic code generation.");
	}

	PROCESS_MITIGATION_STRICT_HANDLE_CHECK_POLICY handle_policy {};
	handle_policy.RaiseExceptionOnInvalidHandleReference = true;
	handle_policy.HandleExceptionsPermanentlyEnabled = true;
	if (!SetProcessMitigationPolicy(ProcessStrictHandleCheckPolicy, &handle_policy, sizeof(handle_policy)))
	{
		LastErrorHandle(Error::Level::Log, L"Couldn't enable strict handle checks.");
	}

	PROCESS_MITIGATION_EXTENSION_POINT_DISABLE_POLICY extension_policy {};
	extension_policy.DisableExtensionPoints = true;
	if (!SetProcessMitigationPolicy(ProcessExtensionPointDisablePolicy, &extension_policy, sizeof(extension_policy)))
	{
		LastErrorHandle(Error::Level::Log, L"Couldn't disable extension point DLLs.");
	}

	PROCESS_MITIGATION_BINARY_SIGNATURE_POLICY signature_policy {};
	signature_policy.MitigationOptIn = true;
	if (!SetProcessMitigationPolicy(ProcessSignaturePolicy, &signature_policy, sizeof(signature_policy)))
	{
		LastErrorHandle(Error::Level::Log, L"Couldn't enable image signature enforcement.");
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
		LastErrorHandle(Error::Level::Log, L"Unable to get volume path name.");
	}

	if (!SetProcessMitigationPolicy(ProcessImageLoadPolicy, &load_policy, sizeof(load_policy)))
	{
		LastErrorHandle(Error::Level::Log, L"Couldn't set image load policy.");
	}
}

std::pair<std::wstring, HRESULT> win32::GetWindowsBuild()
{
	// Microsoft recommends this themselves
	// https://docs.microsoft.com/en-us/windows/desktop/SysInfo/getting-the-system-version
	wil::unique_cotaskmem_string system32;
	HRESULT hr = SHGetKnownFolderPath(FOLDERID_System, KF_FLAG_DEFAULT, NULL, system32.put());
	if (FAILED(hr))
	{
		return { { }, hr };
	}

	std::filesystem::path kernel32 = system32.get();
	kernel32 /= L"kernel32.dll";

	return GetFileVersion(kernel32);
}

std::pair<std::wstring, HRESULT> win32::GetFileVersion(const std::filesystem::path &file)
{
	DWORD size = GetFileVersionInfoSize(file.c_str(), nullptr);
	if (!size)
	{
		return { { }, HRESULT_FROM_WIN32(GetLastError()) };
	}

	auto data = std::make_unique<std::byte[]>(size);
	if (!GetFileVersionInfo(file.c_str(), 0, size, data.get()))
	{
		return { { }, HRESULT_FROM_WIN32(GetLastError()) };
	}

	wchar_t *fileVersion;
	unsigned int length;
	if (!VerQueryValue(data.get(), LR"(\StringFileInfo\040904b0\FileVersion)", reinterpret_cast<void **>(&fileVersion), &length))
	{
		return { { }, HRESULT_FROM_WIN32(GetLastError()) };
	}

	return { { fileVersion, length - 1 }, S_OK };
}

std::wstring_view win32::GetProcessorArchitecture()
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
	wil::unique_itemidlist list(ILCreateFromPath(file.c_str()));

	if (list)
	{
		const HRESULT hr = SHOpenFolderAndSelectItems(list.get(), 0, nullptr, 0);
		if (FAILED(hr))
		{
			std::thread([file, hr]
			{
				std::wstring boxbuffer =
					L"Failed to reveal file \"" + file.native() + L"\"." +
					L"\n\n" + Error::ExceptionFromHRESULT(hr) +
					L"\n\nCopy the file location to the clipboard?";

				if (MessageBox(Window::NullWindow, boxbuffer.c_str(), NAME L" - Error", MB_ICONWARNING | MB_YESNO | MB_SETFOREGROUND) == IDYES)
				{
					CopyToClipboard(file.native());
				}
			}).detach();
		}
	}
}
