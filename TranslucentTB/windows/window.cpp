#include "window.hpp"
#include <optional>
#include <ShObjIdl.h>
#include <wil/com.h>
#include <wil/resource.h>

#include "../../ProgramLog/error.hpp"
#include "win32.hpp"

std::wstring Window::title() const
{
	std::wstring windowTitle;

	const int titleSize = GetWindowTextLength(m_WindowHandle);
	if (!titleSize)
	{
		LastErrorHandle(spdlog::level::info, L"Getting size of title of a window failed.");
		return windowTitle;
	}

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

	const int count = GetClassName(m_WindowHandle, className.data(), 257);
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
	HresultHandle(hr, spdlog::level::info, L"Getting file name of a window failed.");

	return loc;
}

bool Window::on_current_desktop() const
{
	static const auto desktop_manager = wil::CoCreateInstance<VirtualDesktopManager, IVirtualDesktopManager>();

	BOOL on_current_desktop;
	if (HresultHandle(desktop_manager->IsWindowOnCurrentVirtualDesktop(m_WindowHandle, &on_current_desktop), spdlog::level::info, L"Verifying if a window is on the current virtual desktop failed."))
	{
		return on_current_desktop;
	}
	else
	{
		return true;
	}
}
