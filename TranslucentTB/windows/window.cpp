#include "window.hpp"
#include <ShObjIdl.h>
#include <wil/com.h>
#include <wil/resource.h>

#include "../../ProgramLog/error/win32.hpp"
#include "win32.hpp"

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

	// For the null terminator
	std::wstring windowTitle;
	windowTitle.resize(titleSize + 1);

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
	className.resize(257);	// According to docs, maximum length of a class name is 256, but it's ambiguous
							// wether this includes the null terminator or not.

	const int count = GetClassName(m_WindowHandle, className.data(), static_cast<int>(className.size()));
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
	const wil::unique_process_handle processHandle(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, process_id()));
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
		if (const auto on_desktop = on_current_desktop(); !on_desktop.value_or(false))
		{
			return false;
		}

		if (const auto ex_style = long_ptr(GWL_EXSTYLE))
		{
			if (const auto s = *ex_style; s & WS_EX_APPWINDOW || !(s & WS_EX_TOOLWINDOW || s & WS_EX_NOACTIVATE))
			{
				return prop(L"ITaskList_Deleted") == nullptr;
			}
		}
	}

	return false;
}
