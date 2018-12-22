#include "window.hpp"
#include <optional>
#include <ShObjIdl.h>
#include <winrt/base.h>
#include <vector>

#include "createinstance.hpp"
#include "common.hpp"
#include "eventhook.hpp"
#include "ttberror.hpp"

const EventHook Window::m_ChangeHook(EVENT_OBJECT_NAMECHANGE, Window::HandleChangeEvent);
const EventHook Window::m_DestroyHook(EVENT_OBJECT_DESTROY, Window::HandleDestroyEvent);

std::unordered_map<Window, std::wstring> Window::m_ClassNames;
std::unordered_map<Window, std::wstring> Window::m_Filenames;
std::unordered_map<Window, std::wstring> Window::m_Titles;

const Window Window::BroadcastWindow = HWND_BROADCAST;
const Window Window::MessageOnlyWindow = HWND_MESSAGE;

void Window::HandleChangeEvent(DWORD, Window window, ...)
{
	m_Titles.erase(window);
}

void Window::HandleDestroyEvent(DWORD, Window window, ...)
{
	m_Titles.erase(window);
	m_ClassNames.erase(window);
	m_Filenames.erase(window);
}

void Window::ClearCache()
{
	m_ClassNames.clear();
	m_Filenames.clear();
	m_Titles.clear();
}

const std::wstring &Window::title() const
{
	if (m_Titles.count(m_WindowHandle) == 0)
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
		return m_Titles[m_WindowHandle] = std::move(windowTitle);
	}
	else
	{
		return m_Titles.at(m_WindowHandle);
	}
}

const std::wstring &Window::classname() const
{
	if (m_ClassNames.count(m_WindowHandle) == 0)
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
		return m_ClassNames[m_WindowHandle] = std::move(className);
	}
	else
	{
		return m_ClassNames.at(m_WindowHandle);
	}
}

const std::wstring &Window::filename() const
{
	if (m_Filenames.count(m_WindowHandle) == 0)
	{
		DWORD pid;
		GetWindowThreadProcessId(m_WindowHandle, &pid);

		const winrt::handle processHandle(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, pid));
		if (!processHandle)
		{
			LastErrorHandle(Error::Level::Log, L"Getting process handle of a window failed.");
			return m_Filenames[m_WindowHandle] = L"";
		}

		auto [loc, hr] = win32::GetProcessFileName(processHandle.get());
		if (SUCCEEDED(hr))
		{
			loc.erase(0, loc.find_last_of(LR"(/\)") + 1);
		}
		else
		{
			ErrorHandle(hr, Error::Level::Log, L"Getting file name of a window failed.");
		}

		return m_Filenames[m_WindowHandle] = std::move(loc);
	}
	else
	{
		return m_Filenames.at(m_WindowHandle);
	}
}

bool Window::on_current_desktop() const
{
	static auto desktop_manager = create_instance<IVirtualDesktopManager>(CLSID_VirtualDesktopManager);

	BOOL on_current_desktop;
	if (desktop_manager && ErrorHandle(desktop_manager->IsWindowOnCurrentVirtualDesktop(m_WindowHandle, &on_current_desktop), Error::Level::Log, L"Verifying if a window is on the current virtual desktop failed."))
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