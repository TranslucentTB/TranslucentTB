#pragma once
#include "arch.h"
#include <string>
#include <windef.h>

class WindowHelper {

public:
	static std::wstring GetWindowTitle(const HWND &hwnd);
	static std::wstring GetWindowClass(const HWND &hwnd);
	static std::wstring GetWindowFile(const HWND &hwnd);
	static bool IsWindowOnCurrentDesktop(const HWND &hWnd);
	static bool IsWindowMaximised(const HWND &hWnd);
	static bool IsWindowCloaked(const HWND &hWnd);

};