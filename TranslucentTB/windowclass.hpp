#pragma once
#include "arch.h"
#include <functional>
#include <windef.h>
#include <WinUser.h>

class WindowClass {

private:
	std::wstring m_ClassName;
	WNDCLASSEX m_ClassStruct;

public:
	WindowClass(const std::function<long(HWND, unsigned int, WPARAM, LPARAM)> &callback, const std::wstring &className, const wchar_t *iconResource, const unsigned int &style = 0, const HINSTANCE &hInstance = GetModuleHandle(NULL), const HBRUSH &brush = reinterpret_cast<HBRUSH>(COLOR_BACKGROUND), const HCURSOR &cursor = LoadCursor(NULL, IDC_ARROW));
	std::wstring name();
	inline WindowClass(const WindowClass &) = delete;
	inline WindowClass &operator =(const WindowClass &) = delete;
	~WindowClass();
};