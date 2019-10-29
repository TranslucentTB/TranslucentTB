#include "windowclass.hpp"
#include <CommCtrl.h>
#include <WinBase.h>
#include <winerror.h>

#include "../../ProgramLog/error.hpp"
#include "window.hpp"

std::unordered_map<ATOM, WindowClass::callback_t> WindowClass::m_CallbackMap;

LRESULT WindowClass::RawWindowProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	ATOM atom = static_cast<ATOM>(GetClassLongPtr(hwnd, GCW_ATOM));
	if (!atom)
	{
		LastErrorHandle(spdlog::level::critical, L"Failed to get class atom!");
	}

	return m_CallbackMap.at(atom)(hwnd, msg, wParam, lParam);
}

void WindowClass::LoadIcons(const wchar_t *iconResource)
{
	HresultVerify(LoadIconMetric(m_hInstance, iconResource, LIM_LARGE, m_hIcon.put()), spdlog::level::warn, L"Failed to load large window class icon.");
	HresultVerify(LoadIconMetric(m_hInstance, iconResource, LIM_SMALL, m_hIconSmall.put()), spdlog::level::warn, L"Failed to load small window class icon.");
}

WindowClass::WindowClass(callback_t callback, Util::null_terminated_wstring_view className, const wchar_t *iconResource, unsigned int style, HINSTANCE hInstance, HBRUSH brush, HCURSOR cursor) :
	m_hInstance(hInstance)
{
	LoadIcons(iconResource);

	WNDCLASSEX classStruct = {
		.cbSize = sizeof(classStruct),
		.style = style,
		.lpfnWndProc = RawWindowProcedure,
		.hInstance = hInstance,
		.hIcon = m_hIcon.get(),
		.hCursor = cursor,
		.hbrBackground = brush,
		.lpszClassName = className.c_str(),
		.hIconSm = m_hIconSmall.get()
	};

	m_Atom = RegisterClassEx(&classStruct);
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
	LoadIcons(iconResource);

	SetLastError(NO_ERROR);
	if (!SetClassLongPtr(window, GCLP_HICON, reinterpret_cast<LONG_PTR>(m_hIcon.get())) && GetLastError() != NO_ERROR)
	{
		LastErrorHandle(spdlog::level::warn, L"Failed to change large window class icon.");
	}

	SetLastError(NO_ERROR);
	if (!SetClassLongPtr(window, GCLP_HICONSM, reinterpret_cast<LONG_PTR>(m_hIconSmall.get())) && GetLastError() != NO_ERROR)
	{
		LastErrorHandle(spdlog::level::warn, L"Failed to change small window class icon.");
	}
}

WindowClass::~WindowClass()
{
	m_CallbackMap.erase(m_Atom);
	if (!UnregisterClass(atom(), m_hInstance))
	{
		LastErrorHandle(spdlog::level::info, L"Failed to unregister window class.");
	}
}
