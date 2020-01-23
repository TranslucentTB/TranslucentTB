#include "windowclass.hpp"
#include <CommCtrl.h>
#include <WinBase.h>
#include <winerror.h>

#include "../../ProgramLog/error/win32.hpp"
#include "window.hpp"

void WindowClass::LoadIcons(const wchar_t *iconResource)
{
	HresultVerify(LoadIconMetric(m_hInstance, iconResource, LIM_LARGE, m_hIcon.put()), spdlog::level::warn, L"Failed to load large window class icon.");
	HresultVerify(LoadIconMetric(m_hInstance, iconResource, LIM_SMALL, m_hIconSmall.put()), spdlog::level::warn, L"Failed to load small window class icon.");
}

WindowClass::WindowClass(WNDPROC procedure, Util::null_terminated_wstring_view className, const wchar_t *iconResource, HINSTANCE hInstance, unsigned int style, HBRUSH brush, HCURSOR cursor) :
	m_hInstance(hInstance)
{
	LoadIcons(iconResource);

	const WNDCLASSEX classStruct = {
		.cbSize = sizeof(classStruct),
		.style = style,
		.lpfnWndProc = procedure,
		.hInstance = hInstance,
		.hIcon = m_hIcon.get(),
		.hCursor = cursor,
		.hbrBackground = brush,
		.lpszClassName = className.c_str(),
		.hIconSm = m_hIconSmall.get()
	};

	m_Atom = RegisterClassEx(&classStruct);
	if (!m_Atom)
	{
		LastErrorHandle(spdlog::level::critical, L"Failed to register window class!");
	}
}

void WindowClass::ChangeIcon(Window window, const wchar_t *iconResource)
{
	LoadIcons(iconResource);

	SetLastError(NO_ERROR);
	if (!SetClassLongPtr(window, GCLP_HICON, reinterpret_cast<LONG_PTR>(m_hIcon.get())))
	{
		if (const DWORD lastErr = GetLastError(); lastErr != NO_ERROR)
		{
			HresultHandle(HRESULT_FROM_WIN32(lastErr), spdlog::level::warn, L"Failed to change large window class icon.");
		}
	}

	SetLastError(NO_ERROR);
	if (!SetClassLongPtr(window, GCLP_HICONSM, reinterpret_cast<LONG_PTR>(m_hIconSmall.get())))
	{
		if (const DWORD lastErr = GetLastError(); lastErr != NO_ERROR)
		{
			HresultHandle(HRESULT_FROM_WIN32(lastErr), spdlog::level::warn, L"Failed to change small window class icon.");
		}
	}
}

WindowClass::~WindowClass()
{
	if (m_Atom != 0)
	{
		if (!UnregisterClass(atom(), m_hInstance))
		{
			LastErrorHandle(spdlog::level::info, L"Failed to unregister window class.");
		}
	}
}
