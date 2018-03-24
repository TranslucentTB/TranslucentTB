#include "window.hpp"
#include <dwmapi.h>
#include <wrl/wrappers/corewrappers.h>
#include <vector>

#include "ttberror.hpp"

CComPtr<IVirtualDesktopManager> Window::m_DesktopManager;
bool Window::m_DesktopManagerInitFailed;
std::unordered_map<HWND, std::wstring> Window::m_ClassNames;
std::unordered_map<HWND, std::wstring> Window::m_Filenames;

Window Window::Find(const std::wstring &className, const std::wstring &windowName)
{
	return FindWindow(className.empty() ? NULL : className.c_str(), windowName.empty() ? NULL : windowName.c_str());
}

Window::Window(HWND handle)
{
	m_WindowHandle = handle;
}

std::wstring Window::title()
{
	if (m_Title.empty())
	{
		int titleSize = GetWindowTextLength(m_WindowHandle) + 1; // For the null terminator
		std::vector<wchar_t> windowTitleBuffer(titleSize);

		if (!GetWindowText(m_WindowHandle, windowTitleBuffer.data(), titleSize))
		{
			ErrorHandle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Log, L"Getting title of a window failed.");
			return m_Title = L"";
		}

		return m_Title = windowTitleBuffer.data();
	}
	else
	{
		return m_Title;
	}
}

std::wstring Window::classname() const
{
	if (m_ClassNames.count(m_WindowHandle) == 0)
	{
		wchar_t className[256];

		if (!GetClassName(m_WindowHandle, className, 256))
		{
			ErrorHandle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Log, L"Getting class name of a window failed.");
			return m_ClassNames[m_WindowHandle] = L"";
		}
		else
		{
			return m_ClassNames[m_WindowHandle] = className;
		}
	}
	else
	{
		return m_ClassNames.at(m_WindowHandle);
	}
}

std::wstring Window::filename() const
{
	if (m_Filenames.count(m_WindowHandle) == 0)
	{
		namespace wrap = Microsoft::WRL::Wrappers;

		DWORD ProcessId;
		GetWindowThreadProcessId(m_WindowHandle, &ProcessId);

		wrap::HandleT<wrap::HandleTraits::HANDLENullTraits> processHandle(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, ProcessId));
		if (!processHandle.IsValid())
		{
			ErrorHandle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Log, L"Getting process handle of a window failed.");
			return m_Filenames[m_WindowHandle] = L"";
		}

		DWORD path_Size = 33000;
		std::vector<wchar_t> exeName_path(33000);

		if (!QueryFullProcessImageName(processHandle.Get(), 0, exeName_path.data(), &path_Size))
		{
			ErrorHandle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Log, L"Getting file name of a window failed.");
			return m_Filenames[m_WindowHandle] = L"";
		}

		std::wstring exeName(exeName_path.data());
		return m_Filenames[m_WindowHandle] = exeName.substr(exeName.find_last_of(L"/\\") + 1);
	}
	else
	{
		return m_Filenames.at(m_WindowHandle);
	}
}

bool Window::on_current_desktop() const
{
	if (!m_DesktopManagerInitFailed)
	{
		if (!m_DesktopManager)
		{
			m_DesktopManagerInitFailed = !ErrorHandle(m_DesktopManager.CoCreateInstance(CLSID_VirtualDesktopManager), Error::Level::Log, L"Initialization of IVirtualDesktopManager failed.");
		}
	}

	if (m_DesktopManagerInitFailed)
	{
		return true;
	}

	BOOL on_current_desktop;
	HRESULT result = m_DesktopManager->IsWindowOnCurrentVirtualDesktop(m_WindowHandle, &on_current_desktop);

	if (FAILED(result))
	{
		ErrorHandle(result, Error::Level::Log, L"Verifying if a window is on the current virtual desktop failed.");
		return true;
	}
	else
	{
		return on_current_desktop;
	}
}

unsigned int Window::state() const
{
	WINDOWPLACEMENT result;
	if (!GetWindowPlacement(m_WindowHandle, &result))
	{
		ErrorHandle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Log, L"Getting placement of a window failed.");
		return SW_SHOW;
	}
	else
	{
		return result.showCmd;
	}
}

bool Window::visible() const
{
	return IsWindowVisible(m_WindowHandle);
}

HMONITOR Window::monitor() const
{
	return MonitorFromWindow(m_WindowHandle, MONITOR_DEFAULTTOPRIMARY);
}

long Window::send_message(unsigned int message, unsigned int wparam, long lparam) const
{
	return SendMessage(handle(), message, wparam, lparam);
}

HWND Window::handle() const
{
	return m_WindowHandle;
}

Window::operator HWND() const
{
	return handle();
}

template<typename T>
T Window::get_attribute(unsigned long attrib) const
{
	T attribute;
	HRESULT status = DwmGetWindowAttribute(m_WindowHandle, attrib, &attribute, sizeof(attribute));
	ErrorHandle(status, Error::Level::Log, L"Getting attribute of a window failed.");
	return attribute;
}

template BOOL Window::get_attribute<BOOL>(unsigned long attrib) const;
template RECT Window::get_attribute<RECT>(unsigned long attrib) const;