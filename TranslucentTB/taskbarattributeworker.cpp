#include "taskbarattributeworker.hpp"
#include "constants.hpp"
#include "../ExplorerDetour/hook.hpp"
#include "ttberror.hpp"
#include "ttblog.hpp"
#include "win32.hpp"
#include "windows/windowhelper.hpp"

const PFN_SET_WINDOW_COMPOSITION_ATTRIBUTE TaskbarAttributeWorker::SetWindowCompositionAttribute =
	reinterpret_cast<PFN_SET_WINDOW_COMPOSITION_ATTRIBUTE>(GetProcAddress(GetModuleHandle(SWCA_DLL), SWCA_ORDINAL));

void TaskbarAttributeWorker::OnAeroPeekEnterExit(DWORD event, ...)
{
	m_PeekActive = event == EVENT_SYSTEM_PEEKSTART;
	for (const auto &[monitor, _] : m_Taskbars)
	{
		RefreshAttribute(monitor, true);
	}
}

void TaskbarAttributeWorker::OnWindowStateChange(bool skipCheck, DWORD, Window window, LONG idObject, ...)
{
	if (skipCheck || (idObject == OBJID_WINDOW && window.valid()))
	{
		const HMONITOR monitor = window.monitor();
		if (m_Taskbars.contains(monitor))
		{
			InsertWindow(window, true);

			if ((monitor == m_MainTaskbarMonitor && (m_Cfg.Peek == PeekBehavior::WindowMaximisedOnMainMonitor || m_Cfg.Peek == PeekBehavior::DesktopIsForegroundWindow)) ||
				m_Cfg.Peek == PeekBehavior::WindowMaximisedOnAnyMonitor)
			{
				RefreshAeroPeekButton();
			}
			RefreshAttribute(monitor, true);
		}
	}
}

void TaskbarAttributeWorker::OnStartVisibilityChange(bool state)
{
	if (state)
	{
		m_CurrentStartMonitor = GetStartMenuMonitor();
		RefreshAttribute(m_CurrentStartMonitor);
	}
	else
	{
		const HMONITOR old_start_mon = std::exchange(m_CurrentStartMonitor, nullptr);
		RefreshAttribute(old_start_mon);
	}
}

HMONITOR TaskbarAttributeWorker::GetStartMenuMonitor()
{
	return Window::ForegroundWindow().monitor();
}

void TaskbarAttributeWorker::OnForegroundWindowChange(DWORD, Window window, LONG idObject, ...)
{
	if (idObject == OBJID_WINDOW && window.valid())
	{
		if (m_Cfg.Peek == PeekBehavior::DesktopIsForegroundWindow)
		{
			RefreshAeroPeekButton();
		}
	}
}

void TaskbarAttributeWorker::OnWindowCreateDestroy(DWORD event, Window window, LONG idObject, ...)
{
	if (event == EVENT_OBJECT_CREATE)
	{
		if (idObject == OBJID_WINDOW && window.valid())
		{
			OnWindowStateChange(true, event, window, idObject);
			if (window.classname() == SECONDARY_TASKBAR)
			{
				m_Taskbars[window.monitor()] = { window };

				HookTaskbar(window);
			}
		}
	}
	else
	{
		OnWindowStateChange(false, event, window, idObject);
	}
}

void TaskbarAttributeWorker::RefreshTaskbars()
{
	if (m_Cfg.VerboseLog)
	{
		Log::OutputMessage(L"Refreshing taskbar handles.");
	}

	// Older handles are invalid, so clear state to be ready for new ones.
	m_Taskbars.clear();
	m_NormalTaskbars.clear();

	// Keep old hooks alive while we rehook to keep the DLL loaded in Explorer.
	// They will unhook automatically at the end of this function.
	const auto hooks = std::move(m_Hooks);
	m_Hooks.clear(); // Bring back m_Hooks to a known state after being moved from.

	const Window main_taskbar = Window::Find(TASKBAR);
	m_MainTaskbarMonitor = main_taskbar.monitor();
	m_Taskbars[m_MainTaskbarMonitor] = { main_taskbar };
	HookTaskbar(main_taskbar);

	for (const Window secondtaskbar : Window::FindEnum(SECONDARY_TASKBAR))
	{
		m_Taskbars[secondtaskbar.monitor()] = { secondtaskbar };
		HookTaskbar(secondtaskbar);
	}
}

void TaskbarAttributeWorker::HookTaskbar(Window window)
{
	const auto [hook, hr] = Hook::HookExplorer(window);
	if (SUCCEEDED(hr))
	{
		m_Hooks.emplace_back(hook);
	}
	else
	{
		ErrorHandle(hr, Error::Level::Fatal, L"Failed to set hook.");
	}
}

void TaskbarAttributeWorker::InsertWindow(Window window, bool skipCheck)
{
	const HMONITOR monitor = window.monitor();
	if (skipCheck || m_Taskbars.contains(monitor))
	{
		// Note: at() is done here after the check because if not we get some weird crashes when changing DPI.
		if ((WindowHelper::IsUserWindow(window) || m_Cfg.Whitelist.Matches(window)) && !m_Cfg.Blacklist.Matches(window))
		{
			if (window.maximised())
			{
				auto &taskInfo = m_Taskbars.at(monitor);
				taskInfo.MaximisedWindows.insert(window);
				taskInfo.NormalWindows.erase(window);

				return;
			}
			else if (!window.minimised())
			{
				auto &taskInfo = m_Taskbars.at(monitor);
				taskInfo.MaximisedWindows.erase(window);
				taskInfo.NormalWindows.insert(window);

				return;
			}
		}

		auto &taskInfo = m_Taskbars.at(monitor);
		taskInfo.MaximisedWindows.erase(window);
		taskInfo.NormalWindows.erase(window);
	}
}

void TaskbarAttributeWorker::Poll()
{
	// TODO: check if user is using areo peek here (if not possible, assume they aren't)
	m_CurrentStartMonitor = nullptr;

	BOOL start_visible;
	if (ErrorHandle(m_IAV->IsLauncherVisible(&start_visible), Error::Level::Log, L"Failed to query launcher visibility state.") && start_visible)
	{
		m_CurrentStartMonitor = GetStartMenuMonitor();
	}

	for (const Window window : Window::FindEnum())
	{
		InsertWindow(window);
	}

	for (const Window window : m_Taskbars[m_MainTaskbarMonitor].NormalWindows)
	{
		Log::OutputMessage(L"");
		Log::OutputMessage(L"Window");
		Log::OutputMessage(window.title());
		Log::OutputMessage(window.classname());
		Log::OutputMessage(window.file().native());
	}

	for (const Window window : m_Taskbars[m_MainTaskbarMonitor].MaximisedWindows)
	{
		Log::OutputMessage(L"");
		Log::OutputMessage(L"Maximised Window");
		Log::OutputMessage(window.title());
		Log::OutputMessage(window.classname());
		Log::OutputMessage(window.file().native());
	}
}

void TaskbarAttributeWorker::SetAttribute(Window window, TaskbarAppearance config)
{
	if (SetWindowCompositionAttribute)
	{
		if (config.Accent == ACCENT_NORMAL)
		{
			// Without this guard, we get reentrancy: sending WM_THEMECHANGED causes a window's
			// state be changed, so the notification is processed and this is called, sending
			// again the same message, and so on.
			if (!m_NormalTaskbars.contains(window))
			{
				m_NormalTaskbars.insert(window);
				window.send_message(WM_THEMECHANGED);
			}
			return;
		}
		else
		{
			m_NormalTaskbars.erase(window);
		}

		ACCENT_POLICY policy = {
			config.Accent,
			2,
			config.Color,
			0
		};

		if (policy.AccentState == ACCENT_ENABLE_ACRYLICBLURBEHIND && policy.GradientColor >> 24 == 0x00)
		{
			// Acrylic mode doesn't likes a completely 0 opacity
			policy.GradientColor = (0x01 << 24) + (policy.GradientColor & 0x00FFFFFF);
		}

		const WINDOWCOMPOSITIONATTRIBDATA data = {
			WCA_ACCENT_POLICY,
			&policy,
			sizeof(policy)
		};

		SetWindowCompositionAttribute(window, &data);
	}
}

TaskbarAppearance TaskbarAttributeWorker::GetConfigForMonitor(HMONITOR monitor, bool skipCheck) const
{
	if (m_Cfg.UseRegularAppearanceWhenPeeking && m_PeekActive)
	{
		return m_Cfg.DesktopAppearance;
	}

	if (m_Cfg.StartOpenedAppearance.Enabled && m_CurrentStartMonitor == monitor)
	{
		return m_Cfg.StartOpenedAppearance;
	}

	if (m_Cfg.MaximisedWindowAppearance.Enabled)
	{
		if ((skipCheck || m_Taskbars.contains(monitor)) &&
			!m_Taskbars.at(monitor).MaximisedWindows.empty())
		{
			return m_Cfg.MaximisedWindowAppearance;
		}
	}

	if (m_Cfg.VisibleWindowAppearance.Enabled)
	{
		if ((skipCheck || m_Taskbars.contains(monitor)) &&
			!m_Taskbars.at(monitor).NormalWindows.empty())
		{
			return m_Cfg.VisibleWindowAppearance;
		}
	}

	return m_Cfg.DesktopAppearance;
}

void TaskbarAttributeWorker::RefreshAttribute(HMONITOR monitor, bool skipCheck)
{
	if (skipCheck || m_Taskbars.contains(monitor))
	{
		SetAttribute(m_Taskbars.at(monitor).TaskbarWindow, GetConfigForMonitor(monitor, true));
	}
}

void TaskbarAttributeWorker::ShowAeroPeekButton(Window taskbar, bool show)
{
	const Window peek = taskbar.find_child(L"TrayNotifyWnd").find_child(L"TrayShowDesktopButtonWClass");

	if (show)
	{
		SetWindowLong(peek, GWL_EXSTYLE, GetWindowLong(peek, GWL_EXSTYLE) & ~WS_EX_LAYERED);
	}
	else
	{
		SetWindowLong(peek, GWL_EXSTYLE, GetWindowLong(peek, GWL_EXSTYLE) | WS_EX_LAYERED);

		SetLayeredWindowAttributes(peek, 0, 0, LWA_ALPHA);
	}
}

void TaskbarAttributeWorker::RefreshAeroPeekButton()
{
	const auto &taskbarInfo = m_Taskbars.at(m_MainTaskbarMonitor);

	switch (m_Cfg.Peek)
	{
	case PeekBehavior::AlwaysShow:
	case PeekBehavior::AlwaysHide:
		ShowAeroPeekButton(taskbarInfo.TaskbarWindow, m_Cfg.Peek == PeekBehavior::AlwaysShow);
		break;

	case PeekBehavior::WindowMaximisedOnMainMonitor:
		ShowAeroPeekButton(taskbarInfo.TaskbarWindow, !taskbarInfo.MaximisedWindows.empty());
		break;

	case PeekBehavior::WindowMaximisedOnAnyMonitor:
		ShowAeroPeekButton(taskbarInfo.TaskbarWindow, std::any_of(m_Taskbars.begin(), m_Taskbars.end(), [](const auto &kvp)
		{
			return !kvp.second.MaximisedWindows.empty();
		}));
		break;

	case PeekBehavior::DesktopIsForegroundWindow:
	{
		bool isDesktop = false;
		if (const Window foreground = Window::ForegroundWindow(); foreground.valid() && foreground.monitor() == m_MainTaskbarMonitor)
		{
			isDesktop = foreground == Window::ShellWindow();

			// Consider the taskbar as part of the desktop if there is no maximised window on the main monitor.
			// Consider being on the desktop if the foreground window is cloaked or invisible and there are no maximised windows.
			// Some apps such as Discord exhibit a weird behavior when being closed: the window just becomes invisible and it still is the foreground window.
			if (!isDesktop)
			{
				const auto &mainMonInfo = m_Taskbars.at(m_MainTaskbarMonitor);
				isDesktop = (foreground == mainMonInfo.TaskbarWindow || !foreground.visible() || foreground.cloaked()) && mainMonInfo.MaximisedWindows.empty();
			}
		}

		ShowAeroPeekButton(
			taskbarInfo.TaskbarWindow,
			!isDesktop
		);
		break;
	}
	}
}

long TaskbarAttributeWorker::OnRequestAttributeRefresh(WPARAM, LPARAM lParam)
{
	if (m_disableAttributeRefreshReply)
	{
		return 0;
	}

	if (const Window window = reinterpret_cast<HWND>(lParam); m_Taskbars.contains(window.monitor()))
	{
		if (const auto taskbar = m_Taskbars.at(window.monitor()).TaskbarWindow; taskbar == window)
		{
			if (const auto config = GetConfigForMonitor(taskbar.monitor(), true); config.Accent != ACCENT_NORMAL)
			{
				SetAttribute(taskbar, config);
				return 1;
			}
		}
	}

	return 0;
}

void TaskbarAttributeWorker::ReturnToStock()
{
	ShowAeroPeekButton(m_Taskbars.at(m_MainTaskbarMonitor).TaskbarWindow, true);

	m_disableAttributeRefreshReply = true;

	const auto scope_guard = wil::scope_exit([this]
	{
		m_disableAttributeRefreshReply = false;
	});

	for (const auto &[_, monInf] : m_Taskbars)
	{
		SetAttribute(monInf.TaskbarWindow, { ACCENT_NORMAL });
	}
}

TaskbarAttributeWorker::TaskbarAttributeWorker(HINSTANCE hInstance, const Config &cfg) :
	MessageWindow(WORKER_WINDOW, WORKER_WINDOW, hInstance),
	m_PeekActive(false),
	m_PeekUnpeekHook(EVENT_SYSTEM_PEEKSTART, EVENT_SYSTEM_PEEKEND, std::bind(&TaskbarAttributeWorker::OnAeroPeekEnterExit, this, std::placeholders::_1)),
	m_CloakUncloakHook(EVENT_OBJECT_CLOAKED, EVENT_OBJECT_UNCLOAKED, BindHook()),
	m_MinimizeRestoreHook(EVENT_SYSTEM_MINIMIZESTART, EVENT_SYSTEM_MINIMIZEEND, BindHook()),
	m_ResizeMoveHook(EVENT_OBJECT_LOCATIONCHANGE, BindHook()),
	m_ShowHideHook(EVENT_OBJECT_SHOW, EVENT_OBJECT_HIDE, BindHook()),
	m_CurrentStartMonitor(nullptr),
	m_IAV(wil::CoCreateInstance<AppVisibility, IAppVisibility>()),
	m_ForegroundChangeHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, std::bind(&TaskbarAttributeWorker::OnForegroundWindowChange, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)),
	m_MainTaskbarMonitor(nullptr),
	m_CreateDestroyHook(EVENT_OBJECT_CREATE, EVENT_OBJECT_DESTROY, std::bind(&TaskbarAttributeWorker::OnWindowCreateDestroy, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)),
	m_disableAttributeRefreshReply(false),
	m_Cfg(cfg)
{
	const auto av_sink = winrt::make<AppVisibilitySink>(std::bind(&TaskbarAttributeWorker::OnStartVisibilityChange, this, std::placeholders::_1));

	m_IAVECookie.associate(m_IAV.get());
	ErrorHandle(m_IAV->Advise(av_sink.get(), m_IAVECookie.put()), Error::Level::Log, L"Failed to register app visibility sink.");

	RegisterCallback(WM_TTBHOOKREQUESTREFRESH, std::bind(&TaskbarAttributeWorker::OnRequestAttributeRefresh, this, std::placeholders::_1, std::placeholders::_2));

	const auto refresh_taskbars = [this](...)
	{
		ResetState();
		return 0;
	};

	RegisterCallback(WM_DISPLAYCHANGE, refresh_taskbars);
	RegisterCallback(WM_TASKBARCREATED, refresh_taskbars);

	ResetState();
}

void TaskbarAttributeWorker::ResetState()
{
	RefreshTaskbars();
	Poll();
	RefreshAeroPeekButton();
	for (const auto &[monitor, _] : m_Taskbars)
	{
		RefreshAttribute(monitor, true);
	}
}

TaskbarAttributeWorker::~TaskbarAttributeWorker()
{
	ReturnToStock();
}
