#pragma once
#include <set>
#include <ShObjIdl.h>
#include <unordered_map>
#include <utility>
#include <vector>
#include <winrt/base.h>

#include "config.hpp"
#include "eventhook.hpp"
#include "windowshook.hpp"
#include "messagewindow.hpp"
#include "swcadata.hpp"

class TaskbarAttributeWorker : public MessageWindow {
private:
	static const swca::pSetWindowCompositionAttribute SetWindowCompositionAttribute;

	struct MonitorInfo {
		Window TaskbarWindow;
		std::set<Window> MaximisedWindows;
	};

	// todo:
	// dynamic peek button
	// better aero peek support: detect current peeked to window and include in calculation
	// support windows that are immune to peek (with some extended/dwm flag iirc)
	// dynamic cortana and task view
	// color preview cpicker (internal color override?)
	// apply settings & blacklist changes instantly. (PropertyChanged?)
	// correct virtual desktop switch detection
	//     (currently relying on the fact that cloacking status changes)
	//     not sure if an undocumented COM interface is any better
	//     maybe EVENT_SYSTEM_DESKTOPSWITCH
	// support alt-tab? EVENT_SYSTEM_SWITCH{START,END}
	// handle EVENT_OBJECT_SHOW (but what is the reverse that signals it for window objects, EVENT_OBJECT_HIDE doesn't works)
	// on current desktop not working after explorer restart?
	// explorer slows to a crawl when using normal mode. sounds like we have recursion problems

	// Maximised window
	bool m_PeekActive;
	EventHook m_PeekUnpeekHook;
	EventHook m_CloakUncloakHook;
	EventHook m_MinimizeRestoreHook;
	EventHook m_ResizeMoveHook;
	void OnAeroPeekEnterExit(const DWORD event, ...);
	void OnWindowStateChange(const bool skipCheck, DWORD, const Window &window, const LONG idObject, ...);
	static bool IsWindowMaximised(const Window &window);
	static BOOL CALLBACK EnumWindowsProcess(const HWND hWnd, const LPARAM lParam);

	// Start menu
	HMONITOR m_CurrentStartMonitor;
	winrt::com_ptr<IAppVisibility> m_IAV;
	DWORD m_IAVECookie;
	void OnStartVisibilityChange(const bool state);
	HMONITOR GetStartMenuMonitor();

	// Taskbar find & hook
	std::unordered_map<HMONITOR, MonitorInfo> m_Taskbars;
	std::vector<WindowsHook> m_Hooks;
	EventHook m_CreateDestroyHook;
	void OnWindowCreateDestroy(const DWORD event, const Window &window, const LONG idObject, ...);
	void RefreshTaskbars();
	void HookTaskbar(const Window &window);

	// Taskbar appearance refresh
	void Poll();
	bool SetAttribute(const Window &window, const Config::TASKBAR_APPEARANCE &config);
	const Config::TASKBAR_APPEARANCE &GetConfigForMonitor(const HMONITOR monitor, const bool skipCheck = false);
	bool RefreshAttribute(const HMONITOR monitor, const bool skipCheck = false);
	long OnRequestAttributeRefresh(WPARAM, const LPARAM lParam);

	// Other
	bool m_returningToStock;
	EventHook::callback_t BindHook();

public:
	TaskbarAttributeWorker(const HINSTANCE hInstance);

	void ResetState();
	void ReturnToStock();

	~TaskbarAttributeWorker();
};