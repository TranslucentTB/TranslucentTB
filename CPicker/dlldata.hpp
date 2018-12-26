#pragma once
#include "../TranslucentTB/arch.h"
#include <windef.h>

class DllData {
private:
	struct DLGTEMPLATEEX {
		WORD dlgVer, signature;
		DWORD helpID, exStyle, style;
		WORD cDlgItems;
		short x, y, cx, cy;
	};

	static HINSTANCE m_hInst;
	static RECT m_size;

	static RECT CalculateDialogSize(HINSTANCE hInstance);

	friend BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID);

public:
	inline static HINSTANCE GetInstanceHandle()
	{
		return m_hInst;
	}

	inline static const RECT &GetDialogSize()
	{
		return m_size;
	}
};