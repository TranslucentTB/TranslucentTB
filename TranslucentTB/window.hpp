#pragma once
#include <atlbase.h>
#include <dwmapi.h>
#include <ShObjIdl.h>
#include <string>
#include <unordered_map>

class Window {

private:
	static std::unordered_map<Window, std::wstring> m_ClassNames;
	static std::unordered_map<Window, std::wstring> m_Filenames;

protected:
	HWND m_WindowHandle;

public:
	static const Window NullWindow;
	static const Window BroadcastWindow;
	static const Window MessageOnlyWindow;
	static Window Find(const std::wstring &className = L"", const std::wstring &windowName = L"", const Window &parent = Window::NullWindow, const Window &childAfter = Window::NullWindow);
	static Window Create(const unsigned long &dwExStyle, const std::wstring &className,
		const std::wstring &windowName, const unsigned long &dwStyle, const int &x = 0,
		const int &y = 0, const int &nWidth = 0, const int &nHeight = 0, const Window &parent = Window::NullWindow,
		const HMENU &hMenu = NULL, const HINSTANCE &hInstance = GetModuleHandle(NULL), void *lpParam = nullptr);

	constexpr inline Window(const HWND &handle = Window::NullWindow) : m_WindowHandle(handle) { };
	std::wstring title() const;
	const std::wstring &classname() const;
	const std::wstring &filename() const;
	bool on_current_desktop() const;
	unsigned int state() const;
	bool show(int state = SW_SHOW) const;
	bool visible() const;
	WINDOWPLACEMENT placement() const;
	HMONITOR monitor() const;
	long send_message(unsigned int message, unsigned int wparam = 0, long lparam = 0) const;
	long send_message(const std::wstring &message, unsigned int wparam = 0, long lparam = 0) const;
	HWND handle() const;
	operator HWND() const;
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

	friend std::hash<Window>;
};

constexpr Window Window::NullWindow = nullptr;

// Specialize std::hash to allow the use of Window as unordered_map key
namespace std {

	template<> struct hash<Window> {
		std::hash<HWND> m_Hasher;
		inline std::size_t operator()(const Window &k) const
		{
			return m_Hasher(k.m_WindowHandle);
		}
	};

}