#include "window.hpp"
#include <optional>
#include <ShObjIdl.h>
#include <wil/com.h>
#include <wil/resource.h>

#include "../../ProgramLog/error/win32.hpp"
#include "win32.hpp"

std::wstring Window::title() const
{
	std::wstring windowTitle;

	SetLastError(NO_ERROR);
	const int titleSize = GetWindowTextLength(m_WindowHandle);
	if (!titleSize)
	{
		if (const DWORD lastErr = GetLastError(); lastErr != NO_ERROR)
		{
			HresultHandle(HRESULT_FROM_WIN32(lastErr), spdlog::level::info, L"Getting size of title of a window failed.");
		}

		// Failure or empty title.
		return windowTitle;
	}

	// We're assuming that a window won't change title between the previous call and this.
	// But it very well could. It'll either be smaller and waste a bit of RAM, or have
	// GetWindowText trim it.
	
	// For the null terminator
	windowTitle.resize(titleSize + 1);
	const int copiedChars = GetWindowText(m_WindowHandle, windowTitle.data(), titleSize + 1);
	if (!copiedChars)
	{
		LastErrorHandle(spdlog::level::info, L"Getting title of a window failed.");
	}

	windowTitle.resize(copiedChars);
	return windowTitle;
}

std::wstring Window::classname() const
{
	std::wstring className;
	className.resize(257);	// According to docs, maximum length of a class name is 256, but it's ambiguous
							// wether this includes the null terminator or not.

	const int count = GetClassName(m_WindowHandle, className.data(), static_cast<int>(className.size()));
	if (!count)
	{
		LastErrorHandle(spdlog::level::info, L"Getting class name of a window failed.");
	}

	className.resize(count);
	return className;
}

std::filesystem::path Window::file() const
{
	const wil::unique_process_handle processHandle(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, process_id()));
	if (!processHandle)
	{
		LastErrorHandle(spdlog::level::info, L"Getting process handle of a window failed.");
		return { };
	}

	auto [loc, hr] = win32::GetProcessFileName(processHandle.get());
	HresultVerify(hr, spdlog::level::info, L"Getting file name of a window failed.");

	return loc;
}

bool Window::on_current_desktop() const
{
	static const auto desktop_manager = wil::CoCreateInstance<VirtualDesktopManager, IVirtualDesktopManager>();

	BOOL on_current_desktop;
	const HRESULT hr = desktop_manager->IsWindowOnCurrentVirtualDesktop(m_WindowHandle, &on_current_desktop);
	if (SUCCEEDED(hr))
	{
		return on_current_desktop;
	}
	else
	{
		HresultHandle(hr, spdlog::level::info, L"Verifying if a window is on the current virtual desktop failed.");
		return true;
	}
}
