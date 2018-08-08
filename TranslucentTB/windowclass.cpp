#include "windowclass.hpp"
#include <CommCtrl.h>
#include <WinBase.h>
#include <winerror.h>

#include "ttberror.hpp"
#include "window.hpp"

std::unordered_map<ATOM, WindowClass::callback_t> WindowClass::m_CallbackMap;

LRESULT WindowClass::RawWindowProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	ATOM atom = static_cast<ATOM>(GetClassLongPtr(hwnd, GCW_ATOM));
	if (!atom)
	{
		LastErrorHandle(Error::Level::Fatal, L"Failed to get class atom!");
	}

	return m_CallbackMap[atom](hwnd, msg, wParam, lParam);
}

WindowClass::WindowClass(const callback_t &callback, const std::wstring &className, const wchar_t *iconResource, const unsigned int &style, const HINSTANCE &hInstance, const HBRUSH &brush, const HCURSOR &cursor) :
	m_ClassStruct {
		sizeof(m_ClassStruct),
		style,
		RawWindowProcedure,
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

	m_Atom = RegisterClassEx(&m_ClassStruct);
	if (m_Atom)
	{
		m_CallbackMap.emplace(m_Atom, callback);
	}
	else
	{
		LastErrorHandle(Error::Level::Fatal, L"Failed to register window class!");
	}
}

WindowClass::~WindowClass()
{
	m_CallbackMap.erase(m_Atom);
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