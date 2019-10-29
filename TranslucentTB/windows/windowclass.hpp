#pragma once
#include "arch.h"
#include <functional>
#include <string>
#include <unordered_map>
#include <windef.h>
#include <WinUser.h>
#include <wil/resource.h>

#include "util/null_terminated_string_view.hpp"

class Window; // Forward declare to avoid circular deps

class WindowClass {
private:
	using callback_t = std::function<LRESULT(Window, UINT, WPARAM, LPARAM)>;

	ATOM m_Atom;
	HINSTANCE m_hInstance;
	wil::unique_hicon m_hIconSmall, m_hIcon;

	static std::unordered_map<ATOM, callback_t> m_CallbackMap;
	static LRESULT CALLBACK RawWindowProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void LoadIcons(const wchar_t *iconResource);

public:
	WindowClass(callback_t callback, Util::null_terminated_wstring_view className, const wchar_t *iconResource, unsigned int style = 0, HINSTANCE hInstance = GetModuleHandle(nullptr), HBRUSH brush = reinterpret_cast<HBRUSH>(COLOR_BACKGROUND), HCURSOR cursor = LoadCursor(nullptr, IDC_ARROW));

	inline LPCWSTR atom() const noexcept { return reinterpret_cast<LPCWSTR>(static_cast<INT_PTR>(m_Atom)); }

	void ChangeIcon(Window window, const wchar_t *iconResource);

	~WindowClass();

	inline WindowClass(const WindowClass &) = delete;
	inline WindowClass &operator =(const WindowClass &) = delete;
};
