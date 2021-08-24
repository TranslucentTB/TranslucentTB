#pragma once
#include "arch.h"
#include <array>
#include <chrono>
#include <member_thunk/page.hpp>
#include <optional>
#include <ShObjIdl.h>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <wil/com.h>
#include <wil/resource.h>
#include "winrt.hpp"
#include <winrt/TranslucentTB.Xaml.Models.Primitives.h>
#include <winrt/Windows.Internal.Shell.Experience.h> // this is evil >:3

#include "config/config.hpp"
#include "config/taskbarappearance.hpp"
#include "../dynamicloader.hpp"
#include "launchervisibilitysink.hpp"
#include "../windows/messagewindow.hpp"
#include "undoc/user32.hpp"
#include "undoc/uxtheme.hpp"
#include "util/color.hpp"
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

	// fixme before release:
	// - dynamic search on windows 11

	// future improvements:
	// - better aero peek support: detect current peeked to window and include in calculation
	// - notification for owner changes
	// - notification for extended style changes
	// - notification for property changes: this is what is making the discord window not being correctly accounted for after restoring from tray
	// 	   for some reason it shows itself (which we do capture) and then unsets ITaskList_Deleted (which we don't capture)
	// - handle cases where we dont get EVENT_SYSTEM_FOREGROUND when unminimizing a window
	// - add an option for peek to consider main monitor only or all monitors
	// 	   if yes, should always refresh peek whenever anything changes
	// 	   and need some custom logic to check all monitors
	// - make settings optional so that they stack on top of each other?

	// The magic function that does the thing
	const PFN_SET_WINDOW_COMPOSITION_ATTRIBUTE SetWindowCompositionAttribute;
	const PFN_SHOULD_SYSTEM_USE_DARK_MODE ShouldSystemUseDarkMode;

	// State
	bool m_PowerSaver;
	bool m_TaskViewActive;
	bool m_PeekActive;
	bool m_disableAttributeRefreshReply;
	HMONITOR m_CurrentStartMonitor;
	HMONITOR m_CurrentSearchMonitor;
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
	wil::unique_hwineventhook m_TitleChangeHook;
	wil::unique_hwineventhook m_ParentChangeHook;
	std::vector<wil::unique_hhook> m_Hooks;
	wil::unique_hpowernotify m_PowerSaverHook;

	// IAppVisibility
	wil::com_ptr<IAppVisibility> m_IAV;
	wilx::unique_app_visibility_token m_IAVECookie;

	// ICortanaExperienceManager
	winrt::Windows::Internal::Shell::Experience::ICortanaExperienceManager m_SearchManager;
	winrt::event_token m_SuggestionsShownToken, m_SuggestionsHiddenToken;

	// Messages
	std::optional<UINT> m_TaskbarCreatedMessage;
	std::optional<UINT> m_RefreshRequestedMessage;
	std::optional<UINT> m_TaskViewVisibilityChangeMessage;
	std::optional<UINT> m_IsTaskViewOpenedMessage;
	std::optional<UINT> m_StartVisibilityChangeMessage;
	std::optional<UINT> m_SearchVisibilityChangeMessage;

	// Explorer crash detection
	std::chrono::steady_clock::time_point m_LastExplorerRestart;

	// Type aliases
	using taskbar_iterator = std::unordered_map<HMONITOR, MonitorInfo>::iterator;

	// Callbacks
	template<DWORD insert, DWORD remove>
	void CALLBACK WindowInsertRemove(DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD, DWORD);

	void CALLBACK OnAeroPeekEnterExit(DWORD event, HWND, LONG, LONG, DWORD, DWORD);
	void CALLBACK OnWindowStateChange(DWORD, HWND hwnd, LONG idObject, LONG idChild, DWORD, DWORD);
	void CALLBACK OnWindowCreateDestroy(DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD, DWORD);
	void OnStartVisibilityChange(bool state);
	void OnTaskViewVisibilityChange(bool state);
	void OnSearchVisibilityChange(bool state);
	LRESULT OnSystemSettingsChange(UINT uiAction, std::wstring_view changedParameter);
	LRESULT OnPowerBroadcast(const POWERBROADCAST_SETTING *settings);
	LRESULT OnRequestAttributeRefresh(LPARAM lParam);
	LRESULT OnTaskbarCreated();
	LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	// Config
	TaskbarAppearance GetConfig(taskbar_iterator taskbar) const;

	// Color previews
	std::array<std::optional<Util::Color>, 6> m_ColorPreviews;

	// Attribute
	void ShowAeroPeekButton(Window taskbar, bool show);
	void SetAttribute(Window window, TaskbarAppearance config);
	void RefreshAttribute(taskbar_iterator taskbar, std::optional<bool> isMainOpt = std::nullopt);
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
	static bool IsMainTaskbar(Window wnd) noexcept;
	static bool SetNewWindowExStyle(Window wnd, LONG_PTR oldStyle, LONG_PTR newStyle);
	static bool SetContainsValidWindows(std::unordered_set<Window> &set);
	static void DumpWindowSet(std::wstring_view prefix, const std::unordered_set<Window> &set, bool showInfo = true);
	static void DumpWindow(fmt::wmemory_buffer &buf, Window window);
	void CreateAppVisibility();
	void CreateSearchManager();
	void UnregisterSearchCallbacks() noexcept;
	WINEVENTPROC CreateThunk(void (CALLBACK TaskbarAttributeWorker:: *proc)(DWORD, HWND, LONG, LONG, DWORD, DWORD));
	static wil::unique_hwineventhook CreateHook(DWORD eventMin, DWORD eventMax, WINEVENTPROC proc);
	void ReturnToStock();
	bool IsStartMenuOpened() const;
	bool IsSearchOpened() const;
	void InsertTaskbar(HMONITOR mon, Window window);

	inline TaskbarAppearance WithPreview(txmp::TaskbarState state, const TaskbarAppearance &appearance) const
	{
		const auto &preview = m_ColorPreviews.at(static_cast<std::size_t>(state));
		if (preview)
		{
			return { appearance.Accent, *preview, appearance.ShowPeek };
		}
		else
		{
			return appearance;
		}
	}

	inline static HMONITOR GetStartMenuMonitor() noexcept
	{
		// we assume that start is the current foreground window;
		// haven't seen a case where that wasn't true yet.
		// NOTE: this only stands *when* we get notified that
		// start has opened (and as long as it is). when we get
		// notified that it's closed another window may be the
		// foreground window already (eg the user dismissed start
		// by clicking on a window)
		return Window::ForegroundWindow().monitor();
	}

	inline static HMONITOR GetSearchMonitor() noexcept
	{
		// same assumption for search
		return Window::ForegroundWindow().monitor();
	}

	inline static wil::unique_hwineventhook CreateHook(DWORD event, WINEVENTPROC proc)
	{
		return CreateHook(event, event, proc);
	}

public:
	TaskbarAttributeWorker(const Config &cfg, HINSTANCE hInstance, DynamicLoader &loader);

	inline void ConfigurationChanged()
	{
		RefreshAllAttributes();
	}

	void ApplyColorPreview(txmp::TaskbarState state, Util::Color color)
	{
		m_ColorPreviews.at(static_cast<std::size_t>(state)) = color;
		ConfigurationChanged();
	}

	inline void RemoveColorPreview(txmp::TaskbarState state)
	{
		m_ColorPreviews.at(static_cast<std::size_t>(state)).reset();
		ConfigurationChanged();
	}

	void DumpState();
	void ResetState(bool rehook = true);

	~TaskbarAttributeWorker() noexcept(false);
};
