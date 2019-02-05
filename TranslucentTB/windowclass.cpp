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

void WindowClass::LoadIcons(const wchar_t *iconResource)
{
	ErrorHandle(LoadIconMetric(m_ClassStruct.hInstance, iconResource, LIM_LARGE, &m_ClassStruct.hIcon), Error::Level::Log, L"Failed to load large window class icon.");
	ErrorHandle(LoadIconMetric(m_ClassStruct.hInstance, iconResource, LIM_SMALL, &m_ClassStruct.hIconSm), Error::Level::Log, L"Failed to load small window class icon.");
}

void WindowClass::DestroyIcons()
{
	if (!DestroyIcon(m_ClassStruct.hIcon))
	{
		LastErrorHandle(Error::Level::Log, L"Failed to destroy large window class icon.");
	}
	if (!DestroyIcon(m_ClassStruct.hIconSm))
	{
		LastErrorHandle(Error::Level::Log, L"Failed to destory small window class icon.");
	}
}

WindowClass::WindowClass(callback_t callback, const std::wstring &className, const wchar_t *iconResource, unsigned int style, HINSTANCE hInstance, HBRUSH brush, HCURSOR cursor) :
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
	LoadIcons(iconResource);

	m_Atom = RegisterClassEx(&m_ClassStruct);
	if (m_Atom)
	{
		m_CallbackMap[m_Atom] = std::move(callback);
	}
	else
	{
		LastErrorHandle(Error::Level::Fatal, L"Failed to register window class!");
	}
}

void WindowClass::ChangeIcon(Window window, const wchar_t *iconResource)
{
	DestroyIcons();
	LoadIcons(iconResource);

	SetClassLongPtr(window, GCLP_HICON, reinterpret_cast<LONG_PTR>(m_ClassStruct.hIcon));
	SetClassLongPtr(window, GCLP_HICONSM, reinterpret_cast<LONG_PTR>(m_ClassStruct.hIconSm));
}

WindowClass::~WindowClass()
{
	m_CallbackMap.erase(m_Atom);
	if (!UnregisterClass(atom(), m_ClassStruct.hInstance))
	{
		LastErrorHandle(Error::Level::Log, L"Failed to unregister window class.");
	}

	DestroyIcons();
}