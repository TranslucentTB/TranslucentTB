#pragma once
#include "arch.h"
#include <atomic>
#include <windef.h>
#include <wil/com.h>
#include <wil/resource.h>

#include "common.hpp"
#include "undoc/explorer.hpp"
#include "wilx.hpp"

class TaskViewVisibilityMonitor {
private:
	static void UnregisterClassFailFast(ATOM atom) noexcept;
	using unique_class_atom_failfast = wilx::unique_any<UnregisterClassFailFast>;

	static void DestroyWindowFailFast(HWND hwnd) noexcept;
	using unique_window_failfast = wilx::unique_any<DestroyWindowFailFast>;

	static void UnregisterSink(IMultitaskingViewVisibilityService* source, DWORD cookie) noexcept;
	using unique_multitasking_view_visibility_token = wil::unique_com_token<IMultitaskingViewVisibilityService, DWORD, decltype(&UnregisterSink), UnregisterSink>;

	static std::atomic<bool> s_ThreadRunning;
	static unique_handle_failfast s_ThreadCleanupEvent;
	static UINT s_TaskViewVisibilityChangeMessage;
	static UINT s_IsTaskViewOpenedMessage;
	static unique_class_atom_failfast s_WindowClassAtom;
	static unique_handle_failfast s_hThread;
	static wil::com_ptr_failfast<IMultitaskingViewVisibilityService> s_ViewService;

	static void ResetViewService() noexcept;
	using unique_view_service = wilx::unique_call<ResetViewService>;

	static unique_view_service LoadViewService() noexcept;
	static unique_multitasking_view_visibility_token RegisterSink() noexcept;

	static void ThreadMain() noexcept;
	static DWORD WINAPI ThreadProc(LPVOID lpParameter) noexcept;
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept;

	static void EndWatcherThread() noexcept;

	static void Install() noexcept;
	static void Uninstall() noexcept;

	friend BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID) noexcept;
};
