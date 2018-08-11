#pragma once
#include <dwmapi.h>
#include <string>
#include <unordered_map>

#include "findwindowiterator.hpp"
#include "windowclass.hpp"

class EventHook; // Forward declare to avoid circular deps

class Window {

private:
	static std::unordered_map<Window, std::wstring> m_ClassNames;
	static std::unordered_map<Window, std::wstring> m_Filenames;
	static std::unordered_map<Window, std::wstring> m_Titles;
	static EventHook m_Hook;
	static void HandleWinEvent(const DWORD event, const Window &window, ...);

protected:
	HWND m_WindowHandle;

public:
	static const Window NullWindow;
	static const Window BroadcastWindow;
	static const Window MessageOnlyWindow;

	class FindEnum {
	private:
		const std::wstring m_class;
		const std::wstring m_name;
		const HWND m_parent;
	public:
		inline FindEnum(const std::wstring &className = L"", const std::wstring &windowName = L"", const Window &parent = Window::NullWindow) :
			m_class(className),
			m_name(windowName),
			m_parent(parent)
		{ }

		inline FindWindowIterator begin()
		{
			return FindWindowIterator(&m_class, &m_name, m_parent);
		}

		inline const FindWindowIterator &end()
		{
			return FindWindowIterator::EndIterator;
		}
	};

	inline static Window Find(const std::wstring &className = L"", const std::wstring &windowName = L"", const Window &parent = Window::NullWindow, const Window &childAfter = Window::NullWindow)
	{
		return FindWindowEx(parent, childAfter, className.empty() ? NULL : className.c_str(), windowName.empty() ? NULL : windowName.c_str());
	}
	inline static Window Create(const unsigned long &dwExStyle, const std::wstring &className,
		const std::wstring &windowName, const unsigned long &dwStyle, const int &x = 0,
		const int &y = 0, const int &nWidth = 0, const int &nHeight = 0, const Window &parent = Window::NullWindow,
		const HMENU &hMenu = NULL, const HINSTANCE &hInstance = GetModuleHandle(NULL), void *lpParam = nullptr)
	{
		return CreateWindowEx(dwExStyle, className.c_str(), windowName.c_str(), dwStyle, x, y, nWidth, nHeight,
			parent, hMenu, hInstance, lpParam);
	}
	inline static Window Create(const unsigned long &dwExStyle, const WindowClass &winClass,
		const std::wstring &windowName, const unsigned long &dwStyle, const int &x = 0,
		const int &y = 0, const int &nWidth = 0, const int &nHeight = 0, const Window &parent = Window::NullWindow,
		const HMENU &hMenu = NULL, const HINSTANCE &hInstance = GetModuleHandle(NULL), void *lpParam = nullptr)
	{
		return CreateWindowEx(dwExStyle, winClass.atom(), windowName.c_str(), dwStyle, x, y, nWidth, nHeight,
			parent, hMenu, hInstance, lpParam);
	}
	inline static Window ForegroundWindow()
	{
		return GetForegroundWindow();
	}

	constexpr Window(const HWND &handle = Window::NullWindow) : m_WindowHandle(handle) { };
	const std::wstring &title() const;
	const std::wstring &classname() const;
	const std::wstring &filename() const;
	bool on_current_desktop() const;
	inline unsigned int state() const
	{
		const WINDOWPLACEMENT result = placement();
		return result.length != 0 ? result.showCmd : SW_SHOW;
	}
	inline bool show(int state = SW_SHOW) const
	{
		return ShowWindow(m_WindowHandle, state);
	}
	inline bool visible() const
	{
		return IsWindowVisible(m_WindowHandle);
	}
	inline bool valid() const
	{
		return IsWindow(m_WindowHandle);
	}
	WINDOWPLACEMENT placement() const;
	inline HMONITOR monitor() const
	{
		return MonitorFromWindow(m_WindowHandle, MONITOR_DEFAULTTOPRIMARY);
	}
	inline long send_message(unsigned int message, unsigned int wparam = 0, long lparam = 0) const
	{
		return SendMessage(m_WindowHandle, message, wparam, lparam);
	}
	inline long send_message(const std::wstring &message, unsigned int wparam = 0, long lparam = 0) const
	{
		return send_message(RegisterWindowMessage(message.c_str()), wparam, lparam);
	}
	inline HWND handle() const
	{
		return m_WindowHandle;
	}
	inline operator HWND() const
	{
		return m_WindowHandle;
	}
	inline bool operator ==(const Window &right) const
	{
		return m_WindowHandle == right.m_WindowHandle;
	}
	inline bool operator !=(const Window &right) const
	{
		return !operator==(right);
	}

	template<typename T>
	T get_attribute(const DWMWINDOWATTRIBUTE &attrib) const;

	friend struct std::hash<Window>;
};

// Specialize std::hash to allow the use of Window as unordered_map key
namespace std {
	template<> struct hash<Window> {
	private:
		inline static const std::hash<HWND> m_Hasher;

	public:
		inline std::size_t operator()(const Window &k) const noexcept
		{
			return m_Hasher(k.m_WindowHandle);
		}
	};
}