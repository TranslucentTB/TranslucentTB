#pragma once
#include <functional>
#include <ShObjIdl.h>
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
#include "windows/messagewindow.hpp"
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
	// support windows that are immune to peek (with some extended/dwm flag iirc)
	// dynamic cortana and task view
	// apply settings & blacklist changes instantly
	// correct virtual desktop switch detection
	//     (currently relying on the fact that cloacking status changes)
	//     not sure if an undocumented COM interface is any better
	//     maybe EVENT_SYSTEM_DESKTOPSWITCH
	// on current desktop not working after explorer restart?
	// close if explorer crashes twice in 30 seconds

	// Maximised window
	bool m_PeekActive;
	EventHook m_PeekUnpeekHook;
	EventHook m_CloakUncloakHook;
	EventHook m_MinimizeRestoreHook;
	EventHook m_ResizeMoveHook;
	EventHook m_ShowHideHook;
	void OnAeroPeekEnterExit(DWORD event, ...);
	void OnWindowStateChange(bool skipCheck, DWORD, Window window, LONG idObject, ...);

	// Start menu
	HMONITOR m_CurrentStartMonitor;
	wil::com_ptr<IAppVisibility> m_IAV;
	wilx::unique_app_visibility_token m_IAVECookie;
	void OnStartVisibilityChange(bool state);
	static HMONITOR GetStartMenuMonitor();

	// Other
	EventHook m_ForegroundChangeHook;
	void OnForegroundWindowChange(DWORD, Window window, LONG idObject, ...);

	// Taskbar find & hook
	HMONITOR m_MainTaskbarMonitor;
	std::unordered_map<HMONITOR, MonitorInfo> m_Taskbars;
	std::vector<wil::unique_hhook> m_Hooks;
	EventHook m_CreateDestroyHook;
	void OnWindowCreateDestroy(DWORD event, Window window, LONG idObject, ...);
	void RefreshTaskbars();
	void HookTaskbar(Window window);

	// Taskbar appearance refresh
	bool m_disableAttributeRefreshReply;
	std::unordered_set<Window> m_NormalTaskbars;
	void InsertWindow(Window window, bool skipCheck = false);
	void Poll();
	void SetAttribute(Window window, TaskbarAppearance config);
	TaskbarAppearance GetConfigForMonitor(HMONITOR monitor, bool skipCheck = false) const;
	void RefreshAttribute(HMONITOR monitor, bool skipCheck = false);
	void ShowAeroPeekButton(Window taskbar, bool show);
	void RefreshAeroPeekButton();
	long OnRequestAttributeRefresh(WPARAM, LPARAM lParam);

	// Other
	void ReturnToStock();

	inline auto BindHook()
	{
		return std::bind(&TaskbarAttributeWorker::OnWindowStateChange, this, false, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	}

	const Config &m_Cfg;

public:
	TaskbarAttributeWorker(HINSTANCE hInstance, const Config &cfg);

	void ResetState();

	~TaskbarAttributeWorker();
};