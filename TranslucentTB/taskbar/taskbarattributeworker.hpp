#pragma once
#include "arch.h"
#include <member_thunk/page.hpp>
#include <optional>
#include <ShObjIdl.h>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <wil/com.h>
#include <wil/resource.h>

#include "config/config.hpp"
#include "config/taskbarappearance.hpp"
#include "launchervisibilitysink.hpp"
#include "../windows/messagewindow.hpp"
#include "undoc/user32.hpp"
#include "wilx.hpp"

class TaskbarAttributeWorker final : public MessageWindow {
private:
	class AttributeRefresher;
	friend AttributeRefresher;

	struct MonitorInfo {
		Window TaskbarWindow;
		std::unordered_set<Window> MaximisedWindows;
		std::unordered_set<Window> NormalWindows;
	};

	// todo:
	// not sure if possible - better aero peek support: detect current peeked to window and include in calculation
	// dynamic cortana using foreground window
	// check timeline's behavior on multi-mon
	// close if explorer crashes twice in 30 seconds
	// make sure opening start while already opened on other monitor works
	// - notification for owner changes
	// - notification for extended style changes
	// - notification for property changes: this is what is making the discord window not being correctly accounted for after restoring from tray
	// 	   for some reason it shows itself (which we do capture) and then unsets ITaskList_Deleted (which we don't capture)
	// - handle cases where we dont get EVENT_SYSTEM_FOREGROUND when unminimizing a window

	// The magic function that does the thing
	const PFN_SET_WINDOW_COMPOSITION_ATTRIBUTE SetWindowCompositionAttribute;

	// State
	bool m_TimelineActive;
	bool m_PeekActive;
	bool m_disableAttributeRefreshReply;
	HMONITOR m_CurrentStartMonitor;
	HMONITOR m_MainTaskbarMonitor;
	Window m_ForegroundWindow;
	std::unordered_map<HMONITOR, MonitorInfo> m_Taskbars;
	std::unordered_set<Window> m_NormalTaskbars;
	const Config &m_Config;

	// Hooks
	member_thunk::page<> m_ThunkPage;
	wil::unique_hwineventhook m_PeekUnpeekHook;
	wil::unique_hwineventhook m_CloakUncloakHook;
	wil::unique_hwineventhook m_MinimizeRestoreHook;
	wil::unique_hwineventhook m_ResizeMoveHook;
	wil::unique_hwineventhook m_ShowHideHook;
	wil::unique_hwineventhook m_CreateDestroyHook;
	wil::unique_hwineventhook m_ForegroundChangeHook;
	wil::unique_hwineventhook m_TitleChangeHook;
	wil::unique_hwineventhook m_ParentChangeHook;
	std::vector<wil::unique_hhook> m_Hooks;

	// IAppVisibility
	wil::com_ptr<IAppVisibility> m_IAV;
	wilx::unique_app_visibility_token m_IAVECookie;

	// Messages
	std::optional<UINT> m_TaskbarCreatedMessage;
	std::optional<UINT> m_RefreshRequestedMessage;
	std::optional<UINT> m_TimelineNotificationMessage;
	std::optional<UINT> m_GetTimelineStatusMessage;

	// Type aliases
	using taskbar_iterator = std::unordered_map<HMONITOR, MonitorInfo>::iterator;

	// Callbacks
	template<DWORD insert, DWORD remove>
	void CALLBACK WindowInsertRemove(DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD, DWORD);

	void CALLBACK OnAeroPeekEnterExit(DWORD event, HWND, LONG, LONG, DWORD, DWORD);
	void CALLBACK OnWindowStateChange(DWORD, HWND hwnd, LONG idObject, LONG idChild, DWORD, DWORD);
	void CALLBACK OnWindowCreateDestroy(DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD, DWORD);
	void CALLBACK OnForegroundWindowChange(DWORD, HWND hwnd, LONG idObject, LONG idChild, DWORD, DWORD);
	void OnStartVisibilityChange(bool state);
	LRESULT OnRequestAttributeRefresh(LPARAM lParam);
	LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	// Config
	TaskbarAppearance GetConfig(taskbar_iterator taskbar) const;

	// Attribute
	void ShowAeroPeekButton(Window taskbar, bool show);
	void SetAttribute(Window window, TaskbarAppearance config);
	void RefreshAttribute(taskbar_iterator taskbar);
	void RefreshAllAttributes();

	// Log
	static void LogWindowInsertion(const std::pair<std::unordered_set<Window>::iterator, bool> &result, std::wstring_view state, HMONITOR mon);
	static void LogWindowRemoval(std::wstring_view state, Window window, HMONITOR mon);
	static void LogWindowRemovalDestroyed(std::wstring_view state, Window window, HMONITOR mon);

	// State
	void InsertWindow(Window window, bool refresh);

	template<void(*logger)(std::wstring_view, Window, HMONITOR) = LogWindowRemoval>
	void RemoveWindow(Window window, taskbar_iterator it, AttributeRefresher &refresher);

	// Other
	static bool SetOnlyContainsValidWindows(std::unordered_set<Window> &set);
	static void DumpWindowSet(std::wstring_view prefix, const std::unordered_set<Window> &set, bool showInfo = true);
	static void DumpWindow(fmt::wmemory_buffer &buf, Window window);
	void CreateAppVisibility();
	WINEVENTPROC CreateThunk(void (CALLBACK TaskbarAttributeWorker:: *proc)(DWORD, HWND, LONG, LONG, DWORD, DWORD));
	static wil::unique_hwineventhook CreateHook(DWORD eventMin, DWORD eventMax, WINEVENTPROC proc);
	void ReturnToStock();
	bool IsStartMenuOpened();
	void InsertTaskbar(HMONITOR mon, Window window);

	inline HMONITOR GetStartMenuMonitor() noexcept
	{
		// we assume that start is the current foreground window;
		// haven't seen a case where that wasn't true yet.
		// NOTE: this only stands *when* we get notified that
		// start has opened (and as long as it is). when we get
		// notified that it's closed another window may be the
		// foreground window already (eg the user dismissed start
		// by clicking on a window)
		return m_ForegroundWindow ? m_ForegroundWindow.monitor() : nullptr;
	}

	inline static wil::unique_hwineventhook CreateHook(DWORD event, WINEVENTPROC proc)
	{
		return CreateHook(event, event, proc);
	}

public:
	TaskbarAttributeWorker(const Config &cfg, HINSTANCE hInstance, PFN_SET_WINDOW_COMPOSITION_ATTRIBUTE swca);

	inline void ConfigurationChanged()
	{
		RefreshAllAttributes();
	}

	void DumpState();
	void ResetState(bool rehook = true);

	~TaskbarAttributeWorker();
};
