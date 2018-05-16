#pragma once
#include "arch.h"
#include <WinBase.h>
#include <windef.h>
#include <winerror.h>

#include "ttberror.hpp"

class File {

private:
	HANDLE m_Handle;

public:
	inline File(HANDLE handle = nullptr) : m_Handle(handle) { }
	inline operator bool() { return m_Handle != nullptr; }
	inline operator HANDLE() { return m_Handle; }
	inline ~File()
	{
		if (m_Handle != nullptr && !CloseHandle(m_Handle))
		{
			LastErrorHandle(Error::Level::Log, L"Failed to close a file handle.");
		}
	}

};