#include "windowclass.hpp"
#include <CommCtrl.h>
#include <WinBase.h>
#include <winerror.h>

#include "ttberror.hpp"

WindowClass::WindowClass(WNDPROC callback, const std::wstring &className, const wchar_t *iconResource, const unsigned int &style, const HINSTANCE &hInstance, const HBRUSH &brush, const HCURSOR &cursor) :
	m_ClassStruct {
		sizeof(m_ClassStruct),
		style,
		callback,
		0,
		0,
		hInstance,
		nullptr,
		cursor,
		brush,
		nullptr,
		className.c_str(),
		nullptr
	}
{
	ErrorHandle(LoadIconMetric(hInstance, iconResource, LIM_LARGE, &m_ClassStruct.hIcon), Error::Level::Log, L"Failed to load large window class icon.");
	ErrorHandle(LoadIconMetric(hInstance, iconResource, LIM_SMALL, &m_ClassStruct.hIconSm), Error::Level::Log, L"Failed to load small window class icon.");

	if (!(m_Atom = RegisterClassEx(&m_ClassStruct)))
	{
		LastErrorHandle(Error::Level::Fatal, L"Failed to register window class!");
	}
}

WindowClass::~WindowClass()
{
	if (!UnregisterClass(atom(), m_ClassStruct.hInstance))
	{
		LastErrorHandle(Error::Level::Log, L"Failed to unregister window class.");
	}

	if (!DestroyIcon(m_ClassStruct.hIcon))
	{
		LastErrorHandle(Error::Level::Log, L"Failed to destroy large window class icon.");
	}
	if (!DestroyIcon(m_ClassStruct.hIconSm))
	{
		LastErrorHandle(Error::Level::Log, L"Failed to destory small window class icon.");
	}
}