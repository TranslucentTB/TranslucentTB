#include "window.hpp"
#include <optional>
#include <ShObjIdl.h>
#include <winrt/base.h>
#include <vector>

#include "createinstance.hpp"
#include "common.hpp"
#include "eventhook.hpp"
#include "ttberror.hpp"

std::unordered_map<Window, std::wstring> Window::m_ClassNames;
std::unordered_map<Window, std::wstring> Window::m_Filenames;
std::unordered_map<Window, std::wstring> Window::m_Titles;
EventHook Window::m_Hook(EVENT_OBJECT_DESTROY, EVENT_OBJECT_NAMECHANGE, Window::HandleWinEvent, WINEVENT_OUTOFCONTEXT);

void Window::HandleWinEvent(const DWORD event, const Window &window, ...)
{
	switch (event)
	{
	case EVENT_OBJECT_DESTROY:
		m_ClassNames.erase(window);
		m_Filenames.erase(window);
		[[fallthrough]];
	case EVENT_OBJECT_NAMECHANGE:
		m_Titles.erase(window);
		break;
	}
}

const Window Window::NullWindow = nullptr;
const Window Window::BroadcastWindow = HWND_BROADCAST;
const Window Window::MessageOnlyWindow = HWND_MESSAGE;

const std::wstring &Window::title() const
{
	if (m_Titles.count(m_WindowHandle) == 0)
	{
		std::wstring windowTitle;
		int titleSize = GetWindowTextLength(m_WindowHandle) + 1; // For the null terminator
		windowTitle.resize(titleSize);

		int copiedChars = GetWindowText(m_WindowHandle, windowTitle.data(), titleSize);
		if (!copiedChars)
		{
			LastErrorHandle(Error::Level::Log, L"Getting title of a window failed.");
			return m_Titles[m_WindowHandle] = L"";
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

		int count = GetClassName(m_WindowHandle, className.data(), 257);
		if (count)
		{
			className.resize(count);
			return m_ClassNames[m_WindowHandle] = std::move(className);
		}
		else
		{
			LastErrorHandle(Error::Level::Log, L"Getting class name of a window failed.");
			return m_ClassNames[m_WindowHandle] = L"";
		}
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
		DWORD ProcessId;
		GetWindowThreadProcessId(m_WindowHandle, &ProcessId);

		winrt::handle processHandle(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, ProcessId));
		if (!processHandle)
		{
			LastErrorHandle(Error::Level::Log, L"Getting process handle of a window failed.");
			return m_Filenames[m_WindowHandle] = L"";
		}

		DWORD path_Size = LONG_PATH;
		std::wstring exeName;
		exeName.resize(path_Size);

		if (!QueryFullProcessImageName(processHandle.get(), 0, exeName.data(), &path_Size))
		{
			LastErrorHandle(Error::Level::Log, L"Getting file name of a window failed.");
			return m_Filenames[m_WindowHandle] = L"";
		}

		exeName.resize(path_Size);
		exeName.erase(0, exeName.find_last_of(LR"(/\)") + 1);
		return m_Filenames[m_WindowHandle] = std::move(exeName);
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
T Window::get_attribute(const DWMWINDOWATTRIBUTE &attrib) const
{
	T attribute;
	HRESULT status = DwmGetWindowAttribute(m_WindowHandle, attrib, &attribute, sizeof(attribute));
	ErrorHandle(status, Error::Level::Log, L"Getting attribute of a window failed.");
	return attribute;
}

template BOOL Window::get_attribute<BOOL>(const DWMWINDOWATTRIBUTE &attrib) const;
template RECT Window::get_attribute<RECT>(const DWMWINDOWATTRIBUTE &attrib) const;