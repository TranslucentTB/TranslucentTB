#pragma once
#include "arch.h"
#include <dwmapi.h>
#include <functional>
#include <string>
#include <windef.h>
#include <winuser.h>

#ifdef _TRANSLUCENTTB_EXE
#include <filesystem>

#include "../TranslucentTB/windows/windowclass.hpp"
#endif

class Window {
private:
	template<DWMWINDOWATTRIBUTE attrib>
	struct attrib_return_type;

	template<>
	struct attrib_return_type<DWMWA_NCRENDERING_ENABLED> {
		using type = BOOL;
	};

	template<>
	struct attrib_return_type<DWMWA_CAPTION_BUTTON_BOUNDS> {
		using type = RECT;
	};

	template<>
	struct attrib_return_type<DWMWA_EXTENDED_FRAME_BOUNDS> {
		using type = RECT;
	};

	template<>
	struct attrib_return_type<DWMWA_CLOAKED> {
		using type = DWORD;
	};

	template<DWMWINDOWATTRIBUTE attrib>
	using attrib_return_t = typename attrib_return_type<attrib>::type;

	template<DWMWINDOWATTRIBUTE attrib>
	inline HRESULT get_attribute(attrib_return_t<attrib> &val) const noexcept
	{
		return DwmGetWindowAttribute(m_WindowHandle, attrib, &val, sizeof(val));
	}

protected:
	HWND m_WindowHandle;

public:
	static const Window NullWindow;
	static const Window BroadcastWindow;
	static const Window MessageOnlyWindow;

	class FindEnum;

	inline static Window Find(const std::wstring &className = { }, const std::wstring &windowName = { }, Window parent = Window::NullWindow, Window childAfter = Window::NullWindow) noexcept
	{
		return FindWindowEx(parent, childAfter, className.empty() ? nullptr : className.c_str(), windowName.empty() ? nullptr : windowName.c_str());
	}

	inline static Window Create(unsigned long dwExStyle, LPCWSTR winClass,
		const std::wstring &windowName, unsigned long dwStyle, int x = CW_USEDEFAULT, int y = CW_USEDEFAULT,
		int nWidth = CW_USEDEFAULT, int nHeight = CW_USEDEFAULT, Window parent = Window::NullWindow,
		HMENU hMenu = nullptr, HINSTANCE hInstance = GetModuleHandle(nullptr), void *lpParam = nullptr) noexcept
	{
		return CreateWindowEx(dwExStyle, winClass, windowName.c_str(), dwStyle, x, y, nWidth, nHeight,
			parent, hMenu, hInstance, lpParam);
	}

	inline static Window ForegroundWindow() noexcept
	{
		return GetForegroundWindow();
	}

	inline static Window DesktopWindow() noexcept
	{
		return GetDesktopWindow();
	}

	inline static Window ShellWindow() noexcept
	{
		return GetShellWindow();
	}

	constexpr Window(HWND handle = Window::NullWindow) noexcept : m_WindowHandle(handle) { };

#ifdef _TRANSLUCENTTB_EXE
	std::wstring title() const;

	std::wstring classname() const;

	std::filesystem::path file() const;

	bool on_current_desktop() const;
#endif

	inline bool cloaked() const noexcept
	{
		DWORD attr;
		return SUCCEEDED(get_attribute<DWMWA_CLOAKED>(attr)) && attr;
	}

	inline bool maximised() const noexcept
	{
		return IsZoomed(m_WindowHandle);
	}

	inline bool minimised() const noexcept
	{
		return IsIconic(m_WindowHandle);
	}

	inline bool show(int state = SW_SHOW) noexcept
	{
		return ShowWindow(m_WindowHandle, state);
	}

	inline bool visible() const noexcept
	{
		return IsWindowVisible(m_WindowHandle);
	}

	inline bool valid() const noexcept
	{
		return IsWindow(m_WindowHandle);
	}

	inline explicit operator bool() const noexcept
	{
		return valid();
	}

	inline HMONITOR monitor() const noexcept
	{
		return MonitorFromWindow(m_WindowHandle, MONITOR_DEFAULTTONULL);
	}

	inline DWORD thread_id() const noexcept
	{
		return GetWindowThreadProcessId(m_WindowHandle, nullptr);
	}

	inline DWORD process_id() const noexcept
	{
		DWORD pid;
		GetWindowThreadProcessId(m_WindowHandle, &pid);
		return pid;
	}

	inline RECT rect()
	{
		RECT result {};
		GetWindowRect(m_WindowHandle, &result);
		return result;
	}

	inline RECT client_rect()
	{
		RECT result {};
		GetClientRect(m_WindowHandle, &result);
		return result;
	}

	inline TITLEBARINFO titlebar_info() const noexcept
	{
		TITLEBARINFO info = { sizeof(info) };
		GetTitleBarInfo(m_WindowHandle, &info);
		return info;
	}

	inline LONG_PTR style() const noexcept
	{
		return GetWindowLongPtr(m_WindowHandle, GWL_STYLE);
	}

	inline LONG_PTR extended_style() const noexcept
	{
		return GetWindowLongPtr(m_WindowHandle, GWL_EXSTYLE);
	}

	inline LRESULT send_message(unsigned int message, WPARAM wparam = 0, LPARAM lparam = 0) const noexcept
	{
		return SendMessage(m_WindowHandle, message, wparam, lparam);
	}

	inline LRESULT send_message(const std::wstring &message, WPARAM wparam = 0, LPARAM lparam = 0) const
	{
		return send_message(RegisterWindowMessage(message.c_str()), wparam, lparam);
	}

	inline bool post_message(unsigned int message, WPARAM wparam = 0, LPARAM lparam = 0) const noexcept
	{
		return PostMessage(m_WindowHandle, message, wparam, lparam);
	}

	inline bool post_message(const std::wstring &message, WPARAM wparam = 0, LPARAM lparam = 0) const
	{
		return post_message(RegisterWindowMessage(message.c_str()), wparam, lparam);
	}

	inline Window find_child(const std::wstring &className = { }, const std::wstring &windowName = { }, Window childAfter = Window::NullWindow) const noexcept
	{
		return Find(className, windowName, *this, childAfter);
	}

	FindEnum find_childs(std::wstring className = { }, std::wstring windowName = { }) const;

	constexpr HWND handle() const noexcept
	{
		return m_WindowHandle;
	}

	constexpr HWND *put() noexcept
	{
		return &m_WindowHandle;
	}

	constexpr operator HWND() const noexcept
	{
		return m_WindowHandle;
	}

	constexpr bool operator ==(Window right) const noexcept
	{
		return m_WindowHandle == right.m_WindowHandle;
	}

	constexpr bool operator !=(Window right) const noexcept
	{
		return !operator==(right);
	}

	friend struct std::hash<Window>;
};

inline constexpr Window Window::NullWindow = nullptr;
inline const Window Window::BroadcastWindow = HWND_BROADCAST;
inline const Window Window::MessageOnlyWindow = HWND_MESSAGE;

// Specialize std::hash to allow the use of Window as unordered_map and unordered_set key.
namespace std {
	template<>
	struct hash<Window> {
		inline std::size_t operator()(Window k) const noexcept
		{
			static constexpr std::hash<HWND> hasher;
			return hasher(k.m_WindowHandle);
		}
	};
}

// Iterator class for FindEnum
class FindWindowIterator {
private:
	const std::wstring *m_class;
	const std::wstring *m_name;
	Window m_parent, m_currentWindow;

	inline void MoveNext()
	{
		m_currentWindow = m_parent.find_child(*m_class, *m_name, m_currentWindow);
	}

	constexpr FindWindowIterator() noexcept :
		m_class(nullptr),
		m_name(nullptr),
		m_parent(Window::NullWindow),
		m_currentWindow(Window::NullWindow)
	{ }

	inline FindWindowIterator(const std::wstring *className, const std::wstring *windowName, Window parent) :
		m_class(className),
		m_name(windowName),
		m_parent(parent),
		m_currentWindow(Window::NullWindow)
	{
		MoveNext();
	}

	friend class Window::FindEnum;

public:
	inline FindWindowIterator &operator ++()
	{
		MoveNext();
		return *this;
	}

	constexpr bool operator ==(const FindWindowIterator &right) const
	{
		return m_currentWindow == right.m_currentWindow;
	}

	constexpr bool operator !=(const FindWindowIterator &right) const
	{
		return !operator==(right);
	}

	constexpr Window operator *() const
	{
		return m_currentWindow;
	}
};

class Window::FindEnum {
private:
	std::wstring m_class, m_name;
	Window m_parent;
public:
	inline FindEnum(std::wstring className = { }, std::wstring windowName = { }, Window parent = Window::NullWindow) noexcept:
		m_class(std::move(className)),
		m_name(std::move(windowName)),
		m_parent(parent)
	{ }

	inline FindWindowIterator begin() const
	{
		return { &m_class, &m_name, m_parent };
	}

	constexpr FindWindowIterator end() const
	{
		return { };
	}
};

inline Window::FindEnum Window::find_childs(std::wstring className, std::wstring windowName) const
{
	return FindEnum(std::move(className), std::move(windowName), *this);
}
