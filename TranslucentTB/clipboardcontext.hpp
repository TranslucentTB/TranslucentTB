#pragma once
#include <WinBase.h>
#include <windef.h>
#include <winerror.h>
#include <WinUser.h>

#include "ttberror.hpp"

class ClipboardContext {

private:
	bool m_Result;

public:
	inline ClipboardContext(HWND owner = nullptr) : m_Result(OpenClipboard(owner)) { }
	inline operator bool() { return m_Result; }
	inline ~ClipboardContext()
	{
		if (m_Result && !CloseClipboard())
		{
			ErrorHandle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Error, L"Failed to close clipboard.");
		}
	}
};