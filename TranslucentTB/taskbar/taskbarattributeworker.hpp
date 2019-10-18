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

#include "appvisibilitysink.hpp"
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
	// rework peek button hiding - allow to interact when not visible, use normalwindow info
	//     to add support to show when windows are displayed but not maximised
	// make sure opening start while already opened on other monitor works

	// State
	bool m_PeekActive;
	bool m_disableAttributeRefreshReply;
	HMONITOR m_CurrentStartMonitor;
	HMONITOR m_MainTaskbarMonitor;
	std::unordered_map<HMONITOR, MonitorInfo> m_Taskbars;
	std::unordered_set<Window> m_NormalTaskbars;

	// Hooks
	EventHook m_PeekUnpeekHook;
	EventHook m_CloakUncloakHook;
	EventHook m_MinimizeRestoreHook;
	EventHook m_ResizeMoveHook;
	EventHook m_ShowHideHook;
	EventHook m_CreateDestroyHook;
	EventHook m_ForegroundChangeHook;
	EventHook m_TitleChangeHook;
	std::vector<wil::unique_hhook> m_Hooks;

	// IAppVisibility
	wil::com_ptr<IAppVisibility> m_IAV;
	wilx::unique_app_visibility_token m_IAVECookie;
	AppVisibilitySink::StartOpened_revoker m_AVSinkRevoker;

	// Config
	const Config &m_Cfg;

	// Type aliases
	using taskbar_iterator = decltype(m_Taskbars)::const_iterator;

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

	// Config
	TaskbarAppearance GetConfig(taskbar_iterator taskbar) const;

	// Attribute
	void SetAttribute(Window window, TaskbarAppearance config);
	void RefreshAttribute(taskbar_iterator taskbar);

	// State
	void Poll();
	void RefreshTaskbars();
	void InsertTaskbar(HMONITOR mon, Window window);
	taskbar_iterator InsertWindow(Window window);

	// Other
	static void DumpWindowSet(std::wstring_view prefix, const std::unordered_set<Window> &set);
	void ReturnToStock();
	bool IsStartMenuOpened();

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
