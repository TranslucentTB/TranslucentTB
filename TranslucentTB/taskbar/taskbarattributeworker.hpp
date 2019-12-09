#pragma once
#include "arch.h"
#include <functional>
#include <member_thunk/common.hpp>
#include <memory>
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
	// close if explorer crashes twice in 30 seconds
	// rework peek button hiding - allow to interact when not visible, use normalwindow info
	//     to add support to show when windows are displayed but not maximised
	// make sure opening start while already opened on other monitor works
	// hook for what windowhelper checks

	// State
	bool m_PeekActive;
	bool m_disableAttributeRefreshReply;
	HMONITOR m_CurrentStartMonitor;
	HMONITOR m_MainTaskbarMonitor;
	std::unordered_map<HMONITOR, MonitorInfo> m_Taskbars;
	std::unordered_set<Window> m_NormalTaskbars;
	const Config &m_Config;

	// Thunks
	using hook_thunk = std::unique_ptr<member_thunk::thunk<WINEVENTPROC>>;
	hook_thunk m_AeroPeekEnterExitThunk;
	hook_thunk m_WindowStateChangeThunk;
	hook_thunk m_WindowCreateDestroyThunk;
	hook_thunk m_ForegroundWindowChangeThunk;

	// Hooks
	wil::unique_hwineventhook m_PeekUnpeekHook;
	wil::unique_hwineventhook m_CloakUncloakHook;
	wil::unique_hwineventhook m_MinimizeRestoreHook;
	wil::unique_hwineventhook m_ResizeMoveHook;
	wil::unique_hwineventhook m_ShowHideHook;
	wil::unique_hwineventhook m_CreateDestroyHook;
	wil::unique_hwineventhook m_ForegroundChangeHook;
	wil::unique_hwineventhook m_TitleChangeHook;
	std::vector<wil::unique_hhook> m_Hooks;

	// IAppVisibility
	wil::com_ptr<IAppVisibility> m_IAV;
	wilx::unique_app_visibility_token m_IAVECookie;
	AppVisibilitySink::StartOpened_revoker m_AVSinkRevoker;

	// Messages
	UINT m_TaskbarCreatedMessage;
	UINT m_RefreshRequestedMessage;

	// Type aliases
	using taskbar_iterator = decltype(m_Taskbars)::const_iterator;

	// Callbacks
	void CALLBACK OnAeroPeekEnterExit(DWORD event, HWND, LONG, LONG, DWORD, DWORD);
	void CALLBACK OnWindowStateChange(DWORD, HWND hwnd, LONG idObject, LONG, DWORD, DWORD);
	void CALLBACK OnWindowCreateDestroy(DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD idEventThread, DWORD dwmsEventTime);
	void CALLBACK OnForegroundWindowChange(DWORD, HWND hwnd, LONG idObject, LONG, DWORD, DWORD);
	void OnStartVisibilityChange(bool state);
	LRESULT OnRequestAttributeRefresh(LPARAM lParam);
	LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

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
	hook_thunk CreateThunk(void (CALLBACK TaskbarAttributeWorker:: *proc)(DWORD, HWND, LONG, LONG, DWORD, DWORD));
	static wil::unique_hwineventhook CreateHook(DWORD eventMin, DWORD eventMax, const hook_thunk &thunk);
	void ReturnToStock();
	bool IsStartMenuOpened();

	inline static HMONITOR GetStartMenuMonitor()
	{
		return Window::ForegroundWindow().monitor();
	}

	inline static wil::unique_hwineventhook CreateHook(DWORD event, const hook_thunk &thunk)
	{
		return CreateHook(event, event, thunk);
	}

public:
	TaskbarAttributeWorker(const Config &cfg, HINSTANCE hInstance);

	void ConfigurationChanged();
	void DumpState();
	void ResetState();

	~TaskbarAttributeWorker();
};
