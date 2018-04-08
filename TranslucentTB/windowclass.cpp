#include "windowclass.hpp"
#include <CommCtrl.h>
#include <WinBase.h>
#include <winerror.h>

#include "ttberror.hpp"

WindowClass::WindowClass(const std::function<long(HWND, unsigned int, WPARAM, LPARAM)> &callback, const std::wstring &className, const wchar_t *iconResource, const unsigned int &style, const HINSTANCE &hInstance, const HBRUSH &brush, const HCURSOR &cursor) :
	m_ClassName(className),
	m_ClassStruct {
		sizeof(m_ClassStruct),
		style,
		*callback.target<WNDPROC>(),
		0,
		0,
		hInstance,
		nullptr,
		cursor,
		brush,
		nullptr,
		m_ClassName.c_str(),
		nullptr
	}
{
	ErrorHandle(LoadIconMetric(hInstance, iconResource, LIM_LARGE, &m_ClassStruct.hIcon), Error::Level::Log, L"Failed to load large window class icon.");
	ErrorHandle(LoadIconMetric(hInstance, iconResource, LIM_SMALL, &m_ClassStruct.hIconSm), Error::Level::Log, L"Failed to load small window class icon.");

	if (!RegisterClassEx(&m_ClassStruct))
	{
		ErrorHandle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Fatal, L"Failed to register window class!");
	}
}

WindowClass::~WindowClass()
{
	if (!UnregisterClass(m_ClassStruct.lpszClassName, m_ClassStruct.hInstance))
	{
		ErrorHandle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Log, L"Failed to unregister window class.");
	}
}
