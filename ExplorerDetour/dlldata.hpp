#pragma once
#include "../TranslucentTB/arch.h"
#include <windef.h>

class DllData {
private:
	static HINSTANCE m_hInst;
	friend BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID);

public:
	inline static const HINSTANCE &GetInstanceHandle()
	{
		return m_hInst;
	}
};