#pragma once
#include "arch.h"
#include <atomic>
#include <windef.h>
#include <wrl/client.h>

#include "undoc/explorer.hpp"

class TimelineVisibilityMonitor {
	static std::atomic<bool> s_ThreadRunning;
	static HANDLE s_ThreadCleanupEvent;
	static UINT s_TimelineNotification;
	static UINT s_GetTimelineStatus;
	static ATOM s_WindowClassAtom;
	static HANDLE s_hThread;
	static Microsoft::WRL::ComPtr<IMultitaskingViewVisibilityService> s_ViewService;

	static HRESULT LoadViewService() noexcept;
	static HRESULT RegisterSink(DWORD &cookie) noexcept;

	static DWORD WINAPI ThreadProc(LPVOID lpParameter) noexcept;
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept;

	static bool SignalWatcherThreadAndWait(DWORD tid) noexcept;
	static bool EndWatcherThread() noexcept;

	static void Install(HINSTANCE hInst) noexcept;
	static void Uninstall(HINSTANCE hInst) noexcept;

	friend class MultitaskingViewVisibilitySink;
	friend BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID) noexcept;
};
