#pragma once
#include "../TranslucentTB/arch.h"
#include <d2d1_3.h>
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