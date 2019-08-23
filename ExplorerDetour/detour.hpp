#pragma once
#include <string>

#include "undoc/swca.hpp"

namespace Detour {
	extern PFN_SET_WINDOW_COMPOSITION_ATTRIBUTE SetWindowCompositionAttribute;
	extern const std::wstring s_WorkerWindow;
	extern UINT s_RequestRefreshMessage;

	BOOL WINAPI SetWindowCompositionAttributeDetour(HWND hWnd, const WINDOWCOMPOSITIONATTRIBDATA* data) noexcept;
	void Install();
	void Uninstall();
}
