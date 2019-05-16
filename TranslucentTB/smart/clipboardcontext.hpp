#pragma once
#include "arch.h"
#include <WinUser.h>

#include "../windows/window.hpp"
#include "../ttberror.hpp"

class ClipboardContext {
private:
	bool m_Result;

public:
	inline ClipboardContext(Window owner = Window::NullWindow) : m_Result(OpenClipboard(owner)) { }
	inline explicit operator bool() const { return m_Result; }
	inline ~ClipboardContext()
	{
		if (m_Result)
		{
			if (!CloseClipboard())
			{
				LastErrorHandle(Error::Level::Error, L"Failed to close clipboard.");
			}
		}
	}

	inline ClipboardContext(const ClipboardContext &) = delete;
	inline ClipboardContext &operator =(const ClipboardContext &) = delete;
};