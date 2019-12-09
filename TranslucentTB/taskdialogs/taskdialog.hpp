#pragma once
#include "arch.h"
#include <string>
#include <string_view>
#include <windef.h>
#include <WinUser.h>
#include <CommCtrl.h>

#include "appinfo.hpp"
#include "../resources/ids.h"
#include "window.hpp"

class TTBTaskDialog {
private:
	std::wstring m_Title;
	std::wstring m_Content;

	static HRESULT CALLBACK RawCallbackProc(HWND hwnd, UINT uNotification, WPARAM wParam, LPARAM lParam, LONG_PTR dwRefData);

protected:
	TASKDIALOGCONFIG m_Cfg;

	inline TTBTaskDialog(std::wstring title, std::wstring content, Window parent, HINSTANCE hInst) :
		m_Title(std::move(title)),
		m_Content(std::move(content)),
		m_Cfg {
			.cbSize = sizeof(m_Cfg),
			.hwndParent = parent,
			.hInstance = hInst,
			.dwFlags = TDF_ENABLE_HYPERLINKS,
			.pszWindowTitle = APP_NAME,
			.pszMainIcon = MAKEINTRESOURCE(IDI_MAINICON),
			.pszMainInstruction = m_Title.c_str(),
			.pszContent = m_Content.c_str(),
			.pfCallback = RawCallbackProc,
			.lpCallbackData = reinterpret_cast<LONG_PTR>(this)
		}
	{ }

	virtual HRESULT CallbackProc(Window, UINT, WPARAM, LPARAM) = 0;
	bool Run(bool &checked);

public:
	TTBTaskDialog(const TTBTaskDialog &) = delete;
	TTBTaskDialog &operator =(const TTBTaskDialog &) = delete;
};
