#pragma once
#include <set>
#include <ShObjIdl.h>
#include <unordered_map>
#include <utility>
#include <vector>
#include <winrt/base.h>

#include "config.hpp"
#include "eventhook.hpp"
#include "hook.hpp"
#include "messagewindow.hpp"

class TaskbarAttributeWorker : public MessageWindow {
private:
	struct MonitorInfo {
		Window TaskbarWindow;
		std::set<Window> MaximisedWindows;
	};

	// todo:
	// dynamic peek button, detect peek dynamic windows
	// dynamic cortana and task view
	// color preview cpicker (internal color override?)
	// apply settings instantly.

	// Maximised window
	EventHook m_CloackedHook;
	EventHook m_UncloackedHook;
	EventHook m_DestroyedHook;
	EventHook m_MinimizedHook;
	EventHook m_UnminimizedHook;
	EventHook m_ResizeMoveHook;
	void OnWindowStateChange(bool skipCheck, DWORD, const Window &window, LONG idObject, ...);
	static bool IsWindowMaximised(const Window &window);
	static BOOL CALLBACK EnumWindowsProcess(const HWND hWnd, const LPARAM lParam);

	// Start menu
	HMONITOR m_CurrentStartMonitor;
	winrt::com_ptr<IAppVisibility> m_IAV;
	DWORD m_IAVECookie;
	void OnStartVisibilityChange(bool state);
	HMONITOR GetStartMenuMonitor();

	// Taskbar find & hook
	std::unordered_map<HMONITOR, MonitorInfo> m_Taskbars;
	std::vector<TTBHook> m_Hooks;
	EventHook m_CreatedHook;
	void OnWindowCreate(DWORD, const Window &window, LONG idObject, ...);
	void RefreshTaskbars();
	void HookTaskbar(const Window &window);

	// Taskbar appearance refresh
	void Poll();
	void ResetState();
	bool SetAttribute(const Window &window, const Config::TASKBAR_APPEARANCE &config);
	const Config::TASKBAR_APPEARANCE &GetConfigForMonitor(HMONITOR monitor, bool skipCheck = false);
	bool RefreshAttribute(HMONITOR monitor, bool skipCheck = false);
	long OnRequestAttributeRefresh(WPARAM, const LPARAM lParam);

	// Other
	bool m_returningToStock;
	EventHook::callback_t BindHook();

public:
	TaskbarAttributeWorker(const HINSTANCE &hInstance);

	void ReturnToStock();

	~TaskbarAttributeWorker();
};