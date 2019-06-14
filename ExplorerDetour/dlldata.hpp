#pragma once
#include "arch.h"
#include <windef.h>

class DllData {
private:
	static HINSTANCE m_hInst;
	friend BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID) noexcept;

public:
	inline static HINSTANCE GetInstanceHandle()
	{
		return m_hInst;
	}
};