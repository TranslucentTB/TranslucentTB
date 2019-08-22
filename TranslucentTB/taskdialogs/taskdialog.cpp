#include "taskdialog.hpp"
#include <utility>

#include "appinfo.hpp"
#include "../resources/ids.h"
#include "../../ProgramLog/error.hpp"
#include "util/strings.hpp"
#include "win32.hpp"

HRESULT TTBTaskDialog::CallbackProc(HWND hwnd, UINT uNotification, WPARAM wParam, LPARAM lParam, LONG_PTR dwRefData)
{
	if (uNotification == TDN_HYPERLINK_CLICKED)
	{
		std::wstring link = reinterpret_cast<const wchar_t *>(lParam);
		// TODO: update (or not since taskdialogs are going away lul)
		if (Util::StringBeginsWithOneOf(link, { L"http://", L"https://" }))
		{
			win32::OpenLink(link);
		}
		else
		{
			win32::RevealFile(std::move(link));
		}
		return S_OK;
	}
	else
	{
		auto pThis = reinterpret_cast<const TTBTaskDialog *>(dwRefData);
		return pThis->m_Callback(hwnd, uNotification, wParam, lParam);
	}
}

TTBTaskDialog::TTBTaskDialog(std::wstring_view title, std::wstring &&content, callback_t callback, Window parent) :
	m_Title(title),
	m_Content(std::move(content)),
	m_Callback(std::move(callback)),
	m_Cfg { sizeof(m_Cfg) }
{
	m_Cfg.hwndParent = parent;
	m_Cfg.hInstance = GetModuleHandle(nullptr);
	m_Cfg.dwFlags = TDF_ENABLE_HYPERLINKS;
	m_Cfg.pszWindowTitle = APP_NAME;
	m_Cfg.pszMainIcon = MAKEINTRESOURCE(IDI_MAINICON);
	m_Cfg.pszMainInstruction = m_Title.c_str();
	m_Cfg.pszContent = m_Content.c_str();
	m_Cfg.pfCallback = CallbackProc;
	m_Cfg.lpCallbackData = reinterpret_cast<LONG_PTR>(this);
}

bool TTBTaskDialog::Run(bool &checked)
{
	BOOL checkedB = checked;
	HRESULT hr = TaskDialogIndirect(&m_Cfg, nullptr, nullptr, &checkedB);
	if (FAILED(hr))
	{
		MessageBox(Window::NullWindow, (L"Failed to open task dialog.\n\n" + Error::MessageFromHRESULT(hr)).c_str(), ERROR_TITLE, MB_ICONWARNING | MB_OK | MB_SETFOREGROUND);
		return false;
	}
	else
	{
		checked = checkedB;
		return true;
	}
}
