#pragma once
#include <functional>
#include <ShObjIdl.h>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <wil/com.h>
#include <wil/resource.h>

#include "config/config.hpp"
#include "config/taskbarappearance.hpp"
#include "eventhook.hpp"
#include "../windows/messagewindow.hpp"
#include "undoc/swca.hpp"
#include "wilx.hpp"

class TaskbarAttributeWorker : public MessageWindow {
private:
	static const PFN_SET_WINDOW_COMPOSITION_ATTRIBUTE SetWindowCompositionAttribute;

	struct MonitorInfo {
		Window TaskbarWindow;
		std::unordered_set<Window> MaximisedWindows;
		std::unordered_set<Window> NormalWindows;
	};

	// todo:
	// better aero peek support: detect current peeked to window and include in calculation
	// dynamic cortana and task view
	// apply settings & blacklist changes instantly
	// close if explorer crashes twice in 30 seconds
	// rework peek button hiding
	// handle title change

	// Hooks
	EventHook m_PeekUnpeekHook;
	EventHook m_CloakUncloakHook;
	EventHook m_MinimizeRestoreHook;
	EventHook m_ResizeMoveHook;
	EventHook m_ShowHideHook;
	EventHook m_CreateDestroyHook;
	EventHook m_ForegroundChangeHook;
	wilx::unique_app_visibility_token m_IAVECookie;
	std::vector<wil::unique_hhook> m_Hooks;

	// State
	bool m_PeekActive;
	bool m_disableAttributeRefreshReply;
	HMONITOR m_CurrentStartMonitor;
	HMONITOR m_MainTaskbarMonitor;
	std::unordered_map<HMONITOR, MonitorInfo> m_Taskbars;
	std::unordered_set<Window> m_NormalTaskbars;

	// Other
	wil::com_ptr<IAppVisibility> m_IAV;
	const Config &m_Cfg;

	// Callbacks
	void OnAeroPeekEnterExit(DWORD event, ...);
	void OnWindowStateChange(DWORD, Window window, LONG idObject, ...);
	void OnWindowCreateDestroy(DWORD event, Window window, LONG idObject, ...);
	void OnForegroundWindowChange(DWORD, Window window, LONG idObject, ...);
	void OnStartVisibilityChange(bool state);
	long OnRequestAttributeRefresh(WPARAM, LPARAM lParam);

	// Aero Peek button
	void ShowAeroPeekButton(Window taskbar, bool show);
	void RefreshAeroPeekButton();

	// Hooks
	void RefreshTaskbars();
	void HookTaskbar(Window window);

	// Config
	TaskbarAppearance GetConfigForMonitor(HMONITOR monitor) const;
	void SetAttribute(Window window, TaskbarAppearance config);
	void RefreshAttribute(HMONITOR monitor);

	// State
	void Poll();
	void InsertWindow(Window window);

	// Other
	static void DumpWindowSet(std::wstring_view prefix, const std::unordered_set<Window> &set);
	void ReturnToStock();

	inline static HMONITOR GetStartMenuMonitor()
	{
		return Window::ForegroundWindow().monitor();
	}

	inline auto BindEventHook(void (TaskbarAttributeWorker:: *callback)(DWORD, Window, LONG, ...))
	{
		return std::bind(callback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	}

public:
	TaskbarAttributeWorker(HINSTANCE hInstance, const Config &cfg);

	void DumpState();
	void ResetState();

	~TaskbarAttributeWorker();
};
