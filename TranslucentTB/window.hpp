#pragma once
#include <atlbase.h>
#include <ShObjIdl.h>
#include <string>
#include <unordered_map>
#include <windef.h>

class Window {

private:
	static std::unordered_map<HWND, std::wstring> m_ClassNames;
	static std::unordered_map<HWND, std::wstring> m_Filenames;

protected:
	HWND m_WindowHandle;

public:
	static Window Find(const std::wstring &className = L"", const std::wstring &windowName = L"");
	static Window FindEx(const Window &parent, const Window &childAfter, const std::wstring &className = L"", const std::wstring &windowName = L"");
	static Window Create(const unsigned long &dwExStyle, const std::wstring &className,
		const std::wstring &windowName, const unsigned long &dwStyle, const int &x = 0,
		const int &y = 0, const int &nWidth = 0, const int &nHeight = 0, const Window &parent = nullptr,
		const HMENU &hMenu = NULL, const HINSTANCE &hInstance = GetModuleHandle(NULL), void *lpParam = nullptr);

	Window(HWND handle = nullptr);
	std::wstring title() const;
	const std::wstring &classname() const;
	const std::wstring &filename() const;
	bool on_current_desktop() const;
	unsigned int state() const;
	bool show(int state = SW_SHOW) const;
	bool visible() const;
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
	T get_attribute(unsigned long attrib) const;

};