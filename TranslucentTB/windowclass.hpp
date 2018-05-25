#pragma once
#include "arch.h"
#include <string>
#include <windef.h>
#include <WinUser.h>

class WindowClass {

private:
	ATOM m_Atom;
	WNDCLASSEX m_ClassStruct;

public:
	WindowClass(WNDPROC callback, const std::wstring &className, const wchar_t *iconResource, const unsigned int &style = 0, const HINSTANCE &hInstance = GetModuleHandle(NULL), const HBRUSH &brush = reinterpret_cast<HBRUSH>(COLOR_BACKGROUND), const HCURSOR &cursor = LoadCursor(NULL, IDC_ARROW));
	inline LPCWSTR atom() const { return reinterpret_cast<LPCWSTR>(MAKELPARAM(m_Atom, 0)); }
	~WindowClass();

	inline WindowClass(const WindowClass &) = delete;
	inline WindowClass &operator =(const WindowClass &) = delete;
};