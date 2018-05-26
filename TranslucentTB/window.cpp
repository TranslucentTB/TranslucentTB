#include "window.hpp"
#include <wrl/wrappers/corewrappers.h>
#include <vector>

#include "common.hpp"
#include "ttberror.hpp"

std::unordered_map<Window, std::wstring> Window::m_ClassNames;
std::unordered_map<Window, std::wstring> Window::m_Filenames;

const Window Window::NullWindow = nullptr;
const Window Window::BroadcastWindow = HWND_BROADCAST;
const Window Window::MessageOnlyWindow = HWND_MESSAGE;

std::wstring Window::title() const
{
	std::wstring windowTitle;
	int titleSize = GetWindowTextLength(m_WindowHandle) + 1; // For the null terminator
	windowTitle.resize(titleSize);

	int copiedChars = GetWindowText(m_WindowHandle, windowTitle.data(), titleSize);
	if (!copiedChars)
	{
		LastErrorHandle(Error::Level::Log, L"Getting title of a window failed.");
		return L"";
	}

	windowTitle.resize(copiedChars);
	return windowTitle;
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

		namespace wrap = Microsoft::WRL::Wrappers;
		wrap::HandleT<wrap::HandleTraits::HANDLENullTraits> processHandle(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, ProcessId));
		if (!processHandle.IsValid())
		{
			LastErrorHandle(Error::Level::Log, L"Getting process handle of a window failed.");
			return m_Filenames[m_WindowHandle] = L"";
		}

		DWORD path_Size = LONG_PATH;
		std::wstring exeName;
		exeName.resize(path_Size);

		if (!QueryFullProcessImageName(processHandle.Get(), 0, exeName.data(), &path_Size))
		{
			LastErrorHandle(Error::Level::Log, L"Getting file name of a window failed.");
			return m_Filenames[m_WindowHandle] = L"";
		}

		exeName.resize(path_Size);
		exeName.erase(0, exeName.find_last_of(LR"(/\)") + 1);
		return m_Filenames[m_WindowHandle] = exeName;
	}
	else
	{
		return m_Filenames.at(m_WindowHandle);
	}
}

bool Window::on_current_desktop() const
{
	static CComPtr<IVirtualDesktopManager> desktop_manager;
	static bool failed = false;

	if (!failed)
	{
		if (!desktop_manager)
		{
			failed = !ErrorHandle(desktop_manager.CoCreateInstance(CLSID_VirtualDesktopManager), Error::Level::Log, L"Initialization of IVirtualDesktopManager failed.");
		}
	}

	BOOL on_current_desktop;
	if (failed || !ErrorHandle(desktop_manager->IsWindowOnCurrentVirtualDesktop(m_WindowHandle, &on_current_desktop), Error::Level::Log, L"Verifying if a window is on the current virtual desktop failed."))
	{
		return true;
	}
	else
	{
		return on_current_desktop;
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