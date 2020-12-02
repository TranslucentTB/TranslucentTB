#pragma once
#include "arch.h"
#include <functional>
#include <string>
#include <windef.h>
#include <WinUser.h>
#include <wil/resource.h>

#include "util/null_terminated_string_view.hpp"

class Window;

class WindowClass {
private:
	ATOM m_Atom;
	HINSTANCE m_hInstance;
	wil::unique_hicon m_hIconSmall, m_hIcon;
	wil::srwlock m_Lock;

	void LoadIcons(const wchar_t *iconResource);
	void Unregister();

public:
	WindowClass(WNDPROC procedure, Util::null_terminated_wstring_view className, const wchar_t *iconResource, HINSTANCE hInstance, unsigned int style = 0, HBRUSH brush = nullptr, HCURSOR cursor = LoadCursor(nullptr, IDC_ARROW));

	inline LPCWSTR atom() const noexcept { return reinterpret_cast<LPCWSTR>(static_cast<INT_PTR>(m_Atom)); }
	inline HINSTANCE hinstance() const noexcept { return m_hInstance; }

	void ChangeIcon(Window window, const wchar_t *iconResource);

	inline WindowClass(const WindowClass &) = delete;
	inline WindowClass &operator =(const WindowClass &) = delete;

	// NOTE: these operators can only be used BEFORE making a window.
	// once a window is made, and as long as it's alive, the WindowClass
	// can't move.
	inline WindowClass(WindowClass &&other) noexcept :
		m_Atom(std::exchange(other.m_Atom, static_cast<ATOM>(0))),
		m_hInstance(std::exchange(other.m_hInstance, nullptr)),
		m_hIconSmall(std::move(other.m_hIconSmall)),
		m_hIcon(std::move(other.m_hIcon))
	{ }

	inline WindowClass& operator =(WindowClass&& other) noexcept
	{
		if (this != &other)
		{
			std::swap(m_Atom, other.m_Atom);
			std::swap(m_hInstance, other.m_hInstance);
			std::swap(m_hIconSmall, other.m_hIconSmall);
			std::swap(m_hIcon, other.m_hIcon);
		}

		return *this;
	}

	inline ~WindowClass()
	{
		if (m_Atom != 0)
		{
			Unregister();
		}
	}
};
