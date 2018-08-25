#include "window.hpp"
#include <optional>
#include <ShObjIdl.h>
#include <winrt/base.h>
#include <vector>

#include "createinstance.hpp"
#include "common.hpp"
#include "eventhook.hpp"
#include "ttberror.hpp"

std::mutex Window::m_ClassNamesLock;
std::unordered_map<Window, std::shared_ptr<std::wstring>> Window::m_ClassNames;

std::mutex Window::m_FilenamesLock;
std::unordered_map<Window, std::shared_ptr<std::wstring>> Window::m_Filenames;

std::mutex Window::m_TitlesLock;
std::unordered_map<Window, std::shared_ptr<std::wstring>> Window::m_Titles;

const Window Window::NullWindow = nullptr;
const Window Window::BroadcastWindow = HWND_BROADCAST;
const Window Window::MessageOnlyWindow = HWND_MESSAGE;

std::shared_ptr<const std::wstring> Window::title() const
{
	std::lock_guard guard(m_TitlesLock);

	if (m_Titles.count(m_WindowHandle) == 0)
	{
		std::shared_ptr<std::wstring> windowTitle = std::make_shared<std::wstring>();
		int titleSize = GetWindowTextLength(m_WindowHandle) + 1; // For the null terminator
		windowTitle->resize(titleSize);

		int copiedChars = GetWindowText(m_WindowHandle, windowTitle->data(), titleSize);
		if (!copiedChars)
		{
			LastErrorHandle(Error::Level::Log, L"Getting title of a window failed.");
			windowTitle->erase();
			return m_Titles[m_WindowHandle] = std::move(windowTitle);
		}

		windowTitle->resize(copiedChars);
		return m_Titles[m_WindowHandle] = std::move(windowTitle);
	}
	else
	{
		return m_Titles.at(m_WindowHandle);
	}
}

std::shared_ptr<const std::wstring> Window::classname() const
{
	std::lock_guard guard(m_ClassNamesLock);

	if (m_ClassNames.count(m_WindowHandle) == 0)
	{
		std::shared_ptr<std::wstring> className = std::make_shared<std::wstring>();
		className->resize(257);	// According to docs, maximum length of a class name is 256, but it's ambiguous
								// wether this includes the null terminator or not.

		int count = GetClassName(m_WindowHandle, className->data(), 257);
		if (count)
		{
			className->resize(count);
			return m_ClassNames[m_WindowHandle] = std::move(className);
		}
		else
		{
			LastErrorHandle(Error::Level::Log, L"Getting class name of a window failed.");
			className->erase();
			return m_ClassNames[m_WindowHandle] = std::move(className);
		}
	}
	else
	{
		return m_ClassNames.at(m_WindowHandle);
	}
}

std::shared_ptr<const std::wstring> Window::filename() const
{
	std::lock_guard guard(m_FilenamesLock);

	if (m_Filenames.count(m_WindowHandle) == 0)
	{
		DWORD pid;
		GetWindowThreadProcessId(m_WindowHandle, &pid);
		std::shared_ptr<std::wstring> exeName = std::make_shared<std::wstring>();

		const winrt::handle processHandle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, pid);
		if (!processHandle)
		{
			LastErrorHandle(Error::Level::Log, L"Getting process handle of a window failed.");
			return m_Filenames[m_WindowHandle] = std::move(exeName);
		}

		DWORD path_Size = LONG_PATH;
		exeName->resize(path_Size);

		if (!QueryFullProcessImageName(processHandle.get(), 0, exeName->data(), &path_Size))
		{
			LastErrorHandle(Error::Level::Log, L"Getting file name of a window failed.");
			exeName->erase();
			return m_Filenames[m_WindowHandle] = std::move(exeName);
		}

		exeName->resize(path_Size);
		exeName->erase(0, exeName->find_last_of(LR"(/\)") + 1);
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