#include "taskdialog.hpp"
#include <utility>

#include "../../ProgramLog/error/win32.hpp"
#include "util/null_terminated_string_view.hpp"
#include "util/strings.hpp"
#include "win32.hpp"

HRESULT TTBTaskDialog::RawCallbackProc(HWND hwnd, UINT uNotification, WPARAM wParam, LPARAM lParam, LONG_PTR dwRefData)
{
	if (uNotification == TDN_HYPERLINK_CLICKED)
	{
		Util::null_terminated_wstring_view link = reinterpret_cast<const wchar_t *>(lParam);
		// TODO: update (or not since taskdialogs are going away lul)
		if (Util::StringBeginsWithOneOf(link, { L"http://", L"https://" }))
		{
			win32::OpenLink(link);
		}
		else
		{
			win32::RevealFile(std::wstring(link));
		}
		// TODO: can use return to report openlink or revealfile failure?
		return S_OK;
	}
	else
	{
		auto that = reinterpret_cast<TTBTaskDialog *>(dwRefData);
		return that->CallbackProc(hwnd, uNotification, wParam, lParam);
	}
}

bool TTBTaskDialog::Run(bool &checked)
{
	BOOL checkedB = checked;
	HRESULT hr = TaskDialogIndirect(&m_Cfg, nullptr, nullptr, &checkedB);
	if (FAILED(hr))
	{
		MessageBoxEx(Window::NullWindow, (L"Failed to open task dialog.\n\n" + Error::MessageFromHRESULT(hr)).c_str(), ERROR_TITLE, MB_ICONWARNING | MB_OK | MB_SETFOREGROUND, MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL));
		return false;
	}
	else
	{
		checked = checkedB;
		return true;
	}
}
