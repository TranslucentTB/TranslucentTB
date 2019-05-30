#pragma once
#include "arch.h"
#include <functional>
#include <string>
#include <string_view>
#include <windef.h>
#include <WinUser.h>
#include <CommCtrl.h>

#include "window.hpp"

class TTBTaskDialog {
private:
	using callback_t = std::function<HRESULT(Window, unsigned int, WPARAM, LPARAM)>;

	std::wstring m_Title;
	std::wstring m_Content;
	callback_t m_Callback;

	static HRESULT CALLBACK CallbackProc(HWND hwnd, UINT uNotification, WPARAM wParam, LPARAM lParam, LONG_PTR dwRefData);
protected:
	TASKDIALOGCONFIG m_Cfg;
	TTBTaskDialog(std::wstring_view title, std::wstring &&content, callback_t callback, Window parent);

	bool Run(bool &checked);

public:
	TTBTaskDialog(const TTBTaskDialog &) = delete;
	TTBTaskDialog &operator =(const TTBTaskDialog &) = delete;
};