#include "windowclass.hpp"
#include <CommCtrl.h>

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
	LoadIconMetric(hInstance, iconResource, LIM_LARGE, &m_ClassStruct.hIcon);
	LoadIconMetric(hInstance, iconResource, LIM_SMALL, &m_ClassStruct.hIconSm);

	RegisterClassEx(&m_ClassStruct);
}

WindowClass::~WindowClass()
{
	UnregisterClass(m_ClassStruct.lpszClassName, m_ClassStruct.hInstance);
}
