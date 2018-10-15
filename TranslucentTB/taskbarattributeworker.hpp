#pragma once
#include <ShObjIdl.h>
#include <unordered_map>
#include <utility>
#include <vector>
#include <winrt/base.h>

#include "config.hpp"
#include "hook.hpp"
#include "messagewindow.hpp"

class TaskbarAttributeWorker : public MessageWindow {
private:
	enum class TaskbarState {
		Regular,
		MaximisedWindow
	};

	// bool m_IsPeeking;
	// bool m_IsTaskViewOrTimelineOpen;
	// bool m_IsFluentAndTimelineSupported
	// todo: a few event hooks probs.

	// Start menu
	HMONITOR m_CurrentStartMonitor;
	winrt::com_ptr<IAppVisibility> m_IAV;
	DWORD m_IAVECookie;
	void OnStartVisibilityChange(bool state);
	HMONITOR GetStartMenuMonitor();

	// Taskbar find & hook
	std::unordered_map<HMONITOR, std::pair<Window, TaskbarState>> m_Taskbars;
	std::vector<TTBHook> m_Hooks;
	void RefreshTaskbars();
	void HookTaskbar(const Window &window);

	// Taskbar appearance refresh
	void Poll();
	void ResetState();
	bool SetAttribute(const Window &window, const Config::TASKBAR_APPEARANCE &config);
	long RefreshAttribute(HMONITOR monitor);

public:
	TaskbarAttributeWorker(const HINSTANCE &hInstance);

	void ReturnToStock();

	~TaskbarAttributeWorker();
};