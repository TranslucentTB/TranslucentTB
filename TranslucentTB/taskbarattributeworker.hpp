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
	HMONITOR m_CurrentStartMonitor;
	// bool m_IsPeeking;
	// bool m_IsTaskViewOrTimelineOpen;
	// bool m_IsFluentAndTimelineSupported
	// todo: a few event hooks probs.
	winrt::com_ptr<IAppVisibility> m_IAV;
	DWORD m_IAVECookie;
	std::unordered_map<HMONITOR, std::pair<Window, TaskbarState>> m_Taskbars;
	std::vector<TTBHook> m_Hooks;

	bool SetAttribute(const Window &window, const Config::TASKBAR_APPEARANCE &config);
	long RefreshAttribute(HMONITOR monitor);

	void OnStartVisibilityChange(bool state);

	HMONITOR GetStartMenuMonitor();

	void Poll();
	void RefreshTaskbars();
	void HookTaskbar(const Window &window);

public:
	TaskbarAttributeWorker(const HINSTANCE &hInstance);

	void ReturnToStock();

	~TaskbarAttributeWorker();
};