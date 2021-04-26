#include "window.hpp"
#include <ShObjIdl.h>
#include <wil/com.h>
#include <wil/resource.h>

#include "undoc/winternl.hpp"
#include "win32.hpp"

std::optional<std::filesystem::path> Window::TryGetNtImageName(DWORD pid)
{
	static const auto NtQuerySystemInformation = []() noexcept -> PFN_NT_QUERY_SYSTEM_INFORMATION
	{
		const auto ntdll = GetModuleHandle(L"ntdll.dll");
		return ntdll ? reinterpret_cast<PFN_NT_QUERY_SYSTEM_INFORMATION>(GetProcAddress(ntdll, "NtQuerySystemInformation")) : nullptr;
	}();

	if (NtQuerySystemInformation)
	{
		SYSTEM_PROCESS_ID_INFORMATION pidInfo = {
#pragma warning(suppress: 4312) // intentional, the structure uses a pointer to store PIDs
			.ProcessId = reinterpret_cast<PVOID>(pid)
		};

		if (NtQuerySystemInformation(static_cast<SYSTEM_INFORMATION_CLASS>(SystemProcessIdInformation), &pidInfo, sizeof(pidInfo), nullptr) == STATUS_INFO_LENGTH_MISMATCH_UNDOC)
		{
			std::wstring buf;
			buf.resize(pidInfo.ImageName.MaximumLength / 2);
			pidInfo.ImageName.Buffer = buf.data();

			// hopefully the process didn't die midway through this lol
			if (NT_SUCCESS(NtQuerySystemInformation(static_cast<SYSTEM_INFORMATION_CLASS>(SystemProcessIdInformation), &pidInfo, sizeof(pidInfo), nullptr)))
			{
				buf.resize(pidInfo.ImageName.Length / 2);
				return std::move(buf);
			}
		}
	}

	return std::nullopt;
}

std::optional<std::wstring> Window::title() const
{
	SetLastError(NO_ERROR);
	const int titleSize = GetWindowTextLength(m_WindowHandle);
	if (!titleSize)
	{
		if (const DWORD lastErr = GetLastError(); lastErr != NO_ERROR)
		{
			HresultHandle(HRESULT_FROM_WIN32(lastErr), spdlog::level::info, L"Getting size of title of a window failed.");
			return std::nullopt;
		}
		else
		{
			return std::make_optional<std::wstring>();
		}
	}

	// We're assuming that a window won't change title between the previous call and this.
	// But it very well could. It'll either be smaller and waste a bit of RAM, or have
	// GetWindowText trim it.
	std::wstring windowTitle;
	windowTitle.resize(titleSize);

	SetLastError(NO_ERROR);
	const int copiedChars = GetWindowText(m_WindowHandle, windowTitle.data(), titleSize + 1);
	if (!copiedChars)
	{
		if (const DWORD lastErr = GetLastError(); lastErr != NO_ERROR)
		{
			HresultHandle(HRESULT_FROM_WIN32(lastErr), spdlog::level::info, L"Getting title of a window failed.");
			return std::nullopt;
		}
	}

	windowTitle.resize(copiedChars);
	return { std::move(windowTitle) };
}

std::optional<std::wstring> Window::classname() const
{
	std::wstring className;
	className.resize(256);

	const int count = GetClassName(m_WindowHandle, className.data(), static_cast<int>(className.size()) + 1);
	if (!count)
	{
		LastErrorHandle(spdlog::level::info, L"Getting class name of a window failed.");
		return std::nullopt;
	}

	className.resize(count);
	return { std::move(className) };
}

std::optional<std::filesystem::path> Window::file() const
{
	const auto pid = process_id();

	if (auto imageName = TryGetNtImageName(pid))
	{
		return { std::move(*imageName) };
	}
	else
	{
		const wil::unique_process_handle processHandle(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, pid));
		if (!processHandle)
		{
			LastErrorHandle(spdlog::level::info, L"Getting process handle of a window failed.");
			return std::nullopt;
		}

		auto [loc, hr] = win32::GetProcessFileName(processHandle.get());
		if (FAILED(hr))
		{
			HresultHandle(hr, spdlog::level::info, L"Getting file name of a window failed.");
			return std::nullopt;
		}

		return { std::move(loc) };
	}
}

std::optional<bool> Window::on_current_desktop() const
{
	static const auto desktop_manager = []() -> wil::com_ptr<IVirtualDesktopManager>
	{
		try
		{
			return wil::CoCreateInstance<IVirtualDesktopManager>(CLSID_VirtualDesktopManager);
		}
		catch (const wil::ResultException &err)
		{
			ResultExceptionHandle(err, spdlog::level::warn, L"Failed to create virtual desktop manager");
			return nullptr;
		}
	}();

	if (desktop_manager)
	{
		BOOL on_current_desktop;
		if (const HRESULT hr = desktop_manager->IsWindowOnCurrentVirtualDesktop(m_WindowHandle, &on_current_desktop); SUCCEEDED(hr))
		{
			return on_current_desktop;
		}
		else
		{
			HresultHandle(hr, spdlog::level::info, L"Verifying if a window is on the current virtual desktop failed.");
		}
	}

	return std::nullopt;
}

bool Window::is_user_window() const
{
	if (valid() && visible() && !cloaked() && ancestor(GA_ROOT) == m_WindowHandle && get(GW_OWNER) == Window::NullWindow)
	{
		const auto ex_style = get_long_ptr(GWL_EXSTYLE);
		if (ex_style &&
			(*ex_style & WS_EX_APPWINDOW || !(*ex_style & (WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE))) &&
			prop(L"ITaskList_Deleted") == nullptr)
		{
			// done last because it's expensive due to being reentrant
			return on_current_desktop().value_or(false);
		}
	}

	return false;
}
