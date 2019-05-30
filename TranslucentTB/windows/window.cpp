#include "window.hpp"
#include <optional>
#include <ShObjIdl.h>
#include <wil/com.h>
#include <wil/resource.h>
#include <vector>

#include "constants.hpp"
#include "../smart/eventhook.hpp"
#include "../ttberror.hpp"
#include "../win32.hpp"

const EventHook Window::s_ChangeHook(EVENT_OBJECT_NAMECHANGE, Window::HandleChangeEvent);
const EventHook Window::s_DestroyHook(EVENT_OBJECT_DESTROY, Window::HandleDestroyEvent);

std::unordered_map<Window, std::wstring> Window::s_ClassNames;
std::unordered_map<Window, std::filesystem::path> Window::s_FilePaths;
std::unordered_map<Window, std::wstring> Window::s_Titles;

void Window::HandleChangeEvent(DWORD, Window window, ...)
{
	s_Titles.erase(window);
}

void Window::HandleDestroyEvent(DWORD, Window window, ...)
{
	s_Titles.erase(window);
	s_ClassNames.erase(window);
	s_FilePaths.erase(window);
}

std::wstring_view Window::title() const
{
	if (!s_Titles.contains(m_WindowHandle))
	{
		std::wstring windowTitle;
		const int titleSize = GetWindowTextLength(m_WindowHandle) + 1; // For the null terminator
		windowTitle.resize(titleSize);

		const int copiedChars = GetWindowText(m_WindowHandle, windowTitle.data(), titleSize);
		if (!copiedChars)
		{
			LastErrorHandle(Error::Level::Log, L"Getting title of a window failed.");
		}

		windowTitle.resize(copiedChars);
		return s_Titles[m_WindowHandle] = std::move(windowTitle);
	}
	else
	{
		return s_Titles.at(m_WindowHandle);
	}
}

std::wstring_view Window::classname() const
{
	if (!s_ClassNames.contains(m_WindowHandle))
	{
		std::wstring className;
		className.resize(257);	// According to docs, maximum length of a class name is 256, but it's ambiguous
								// wether this includes the null terminator or not.

		const int count = GetClassName(m_WindowHandle, className.data(), 257);
		if (!count)
		{
			LastErrorHandle(Error::Level::Log, L"Getting class name of a window failed.");
		}

		className.resize(count);
		return s_ClassNames[m_WindowHandle] = std::move(className);
	}
	else
	{
		return s_ClassNames.at(m_WindowHandle);
	}
}

const std::filesystem::path &Window::file() const
{
	if (!s_FilePaths.contains(m_WindowHandle))
	{
		DWORD pid;
		GetWindowThreadProcessId(m_WindowHandle, &pid);

		const wil::unique_process_handle processHandle(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, pid));
		if (!processHandle)
		{
			LastErrorHandle(Error::Level::Log, L"Getting process handle of a window failed.");
			return s_FilePaths[m_WindowHandle] = std::filesystem::path();
		}

		auto [loc, hr] = win32::GetProcessFileName(processHandle.get());
		if (FAILED(hr))
		{
			ErrorHandle(hr, Error::Level::Log, L"Getting file name of a window failed.");
		}

		return s_FilePaths[m_WindowHandle] = std::move(loc);
	}
	else
	{
		return s_FilePaths.at(m_WindowHandle);
	}
}

bool Window::on_current_desktop() const
{
	static const auto desktop_manager = wil::CoCreateInstance<VirtualDesktopManager, IVirtualDesktopManager>();

	BOOL on_current_desktop;
	if (ErrorHandle(desktop_manager->IsWindowOnCurrentVirtualDesktop(m_WindowHandle, &on_current_desktop), Error::Level::Log, L"Verifying if a window is on the current virtual desktop failed."))
	{
		return on_current_desktop;
	}
	else
	{
		return true;
	}
}

WINDOWPLACEMENT Window::placement() const
{
	WINDOWPLACEMENT result {};
	if (!GetWindowPlacement(m_WindowHandle, &result))
	{
		LastErrorHandle(Error::Level::Log, L"Getting placement of a window failed.");
	}

	return result;
}

template<typename T>
T Window::get_attribute(DWMWINDOWATTRIBUTE attrib) const
{
	T attribute{};
	HRESULT status = DwmGetWindowAttribute(m_WindowHandle, attrib, &attribute, sizeof(attribute));
	ErrorHandle(status, Error::Level::Log, L"Getting attribute of a window failed.");
	return attribute;
}

template BOOL Window::get_attribute<BOOL>(DWMWINDOWATTRIBUTE attrib) const;
template RECT Window::get_attribute<RECT>(DWMWINDOWATTRIBUTE attrib) const;