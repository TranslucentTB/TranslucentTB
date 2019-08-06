#include "windowclass.hpp"
#include <CommCtrl.h>
#include <WinBase.h>
#include <winerror.h>

#include "../log/ttberror.hpp"
#include "window.hpp"

std::unordered_map<ATOM, WindowClass::callback_t> WindowClass::m_CallbackMap;

LRESULT WindowClass::RawWindowProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	ATOM atom = static_cast<ATOM>(GetClassLongPtr(hwnd, GCW_ATOM));
	if (!atom)
	{
		LastErrorHandle(spdlog::level::critical, L"Failed to get class atom!");
	}

	return m_CallbackMap[atom](hwnd, msg, wParam, lParam);
}

void WindowClass::LoadIcons(const wchar_t *iconResource)
{
	HresultHandle(LoadIconMetric(m_ClassStruct.hInstance, iconResource, LIM_LARGE, &m_ClassStruct.hIcon), spdlog::level::warn, L"Failed to load large window class icon.");
	HresultHandle(LoadIconMetric(m_ClassStruct.hInstance, iconResource, LIM_SMALL, &m_ClassStruct.hIconSm), spdlog::level::warn, L"Failed to load small window class icon.");
}

void WindowClass::DestroyIcons()
{
	if (!DestroyIcon(m_ClassStruct.hIcon))
	{
		LastErrorHandle(spdlog::level::warn, L"Failed to destroy large window class icon.");
	}
	if (!DestroyIcon(m_ClassStruct.hIconSm))
	{
		LastErrorHandle(spdlog::level::warn, L"Failed to destroy small window class icon.");
	}
}

WindowClass::WindowClass(callback_t callback, const std::wstring &className, const wchar_t *iconResource, unsigned int style, HINSTANCE hInstance, HBRUSH brush, HCURSOR cursor) :
	m_ClassStruct {
		.cbSize = sizeof(m_ClassStruct),
		.style = style,
		.lpfnWndProc = RawWindowProcedure,
		.hInstance = hInstance,
		.hCursor = cursor,
		.hbrBackground = brush,
		.lpszClassName = className.c_str()
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
		LastErrorHandle(spdlog::level::critical, L"Failed to register window class!");
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
		LastErrorHandle(spdlog::level::info, L"Failed to unregister window class.");
	}

	DestroyIcons();
}
