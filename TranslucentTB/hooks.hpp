#pragma once
#include "eventhook.hpp"
#include "window.hpp"

// TODO: remove this
class Hooks {
private:
	static EventHook m_ChangeHook;
	static EventHook m_DestroyHook;

	static void HandleChangeEvent(const DWORD, const Window &window, ...);
	static void HandleDestroyEvent(const DWORD, const Window &window, ...);
};