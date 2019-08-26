#include "taskbarattributeworker.hpp"

#include "appvisibilitysink.hpp"
#include "constants.hpp"
#include "../ExplorerDetour/explorerdetour.hpp"
#include "../../ProgramLog/error.hpp"
#include "undoc/winuser.hpp"
#include "win32.hpp"
#include "../windows/windowhelper.hpp"

const PFN_SET_WINDOW_COMPOSITION_ATTRIBUTE TaskbarAttributeWorker::SetWindowCompositionAttribute =
	reinterpret_cast<PFN_SET_WINDOW_COMPOSITION_ATTRIBUTE>(GetProcAddress(GetModuleHandle(SWCA_DLL), SWCA_ORDINAL));

void TaskbarAttributeWorker::OnAeroPeekEnterExit(DWORD event, ...)
{
	m_PeekActive = event == EVENT_SYSTEM_PEEKSTART;
	for (const auto &[monitor, _] : m_Taskbars)
	{
		RefreshAttribute(monitor);
	}
}

void TaskbarAttributeWorker::OnWindowStateChange(DWORD, Window window, LONG idObject, ...)
{
	if (idObject == OBJID_WINDOW && window.valid())
	{
		InsertWindow(window);

		const HMONITOR monitor = window.monitor();
		if ((monitor == m_MainTaskbarMonitor && (m_Cfg.Peek == PeekBehavior::WindowMaximisedOnMainMonitor || m_Cfg.Peek == PeekBehavior::DesktopIsForegroundWindow)) ||
			m_Cfg.Peek == PeekBehavior::WindowMaximisedOnAnyMonitor)
		{
			RefreshAeroPeekButton();
		}
		RefreshAttribute(monitor);
	}
}

void TaskbarAttributeWorker::OnWindowCreateDestroy(DWORD event, Window window, LONG idObject, ...)
{
	if (idObject == OBJID_WINDOW && window.valid())
	{
		if (const auto className = window.classname(); className == TASKBAR || className == SECONDARY_TASKBAR)
		{
			MessagePrint(spdlog::level::debug, L"A taskbar got created or destroyed, refreshing...");
			RefreshTaskbars();
		}
	}

	OnWindowStateChange(event, window, idObject);
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

void TaskbarAttributeWorker::OnStartVisibilityChange(bool state)
{
	if (state)
	{
		m_CurrentStartMonitor = GetStartMenuMonitor();
		RefreshAttribute(m_CurrentStartMonitor);

		MessagePrint(spdlog::level::debug, fmt::format(fmt(L"Start menu opened on monitor {}"), static_cast<void *>(m_CurrentStartMonitor)));
	}
	else
	{
		const HMONITOR old_start_mon = std::exchange(m_CurrentStartMonitor, nullptr);
		RefreshAttribute(old_start_mon);

		MessagePrint(spdlog::level::debug, fmt::format(fmt(L"Start menu closed on monitor {}"), static_cast<void *>(old_start_mon)));
	}
}

long TaskbarAttributeWorker::OnRequestAttributeRefresh(WPARAM, LPARAM lParam)
{
	if (m_disableAttributeRefreshReply)
	{
		return 0;
	}

	const Window window = reinterpret_cast<HWND>(lParam);
	if (const auto iter = m_Taskbars.find(window.monitor()); iter != m_Taskbars.end() && iter->second.TaskbarWindow == window)
	{
		if (const auto config = GetConfigForMonitor(iter->first); config.Accent != ACCENT_NORMAL)
		{
			SetAttribute(iter->second.TaskbarWindow, config);
			return 1;
		}
	}

	return 0;
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
	}
}

void TaskbarAttributeWorker::RefreshTaskbars()
{
	MessagePrint(spdlog::level::debug, L"Refreshing taskbar handles.");

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
	auto hook = ExplorerDetour::Inject(window);
	if (hook)
	{
		m_Hooks.push_back(std::move(hook));
	}
	else
	{
		LastErrorHandle(spdlog::level::critical, L"Failed to set hook.");
	}
}

TaskbarAppearance TaskbarAttributeWorker::GetConfigForMonitor(HMONITOR monitor) const
{
	if (m_Cfg.UseRegularAppearanceWhenPeeking && m_PeekActive)
	{
		return m_Cfg.DesktopAppearance;
	}

	if (m_Cfg.StartOpenedAppearance.Enabled && m_CurrentStartMonitor == monitor)
	{
		return m_Cfg.StartOpenedAppearance;
	}

	const auto iter = m_Taskbars.find(monitor);

	if (iter != m_Taskbars.end())
	{
		if (m_Cfg.MaximisedWindowAppearance.Enabled)
		{
			if (!iter->second.MaximisedWindows.empty())
			{
				return m_Cfg.MaximisedWindowAppearance;
			}
		}

		if (m_Cfg.VisibleWindowAppearance.Enabled)
		{
			if (!iter->second.MaximisedWindows.empty() || !iter->second.NormalWindows.empty())
			{
				return m_Cfg.VisibleWindowAppearance;
			}
		}
	}

	return m_Cfg.DesktopAppearance;
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

void TaskbarAttributeWorker::RefreshAttribute(HMONITOR monitor)
{
	if (const auto iter = m_Taskbars.find(monitor); iter != m_Taskbars.end())
	{
		SetAttribute(iter->second.TaskbarWindow, GetConfigForMonitor(monitor));
	}
}

void TaskbarAttributeWorker::Poll()
{
	// TODO: check if user is using areo peek here (if not possible, assume they aren't)

	if (BOOL start_visible; HresultHandle(m_IAV->IsLauncherVisible(&start_visible), spdlog::level::info, L"Failed to query launcher visibility state.") && start_visible)
	{
		m_CurrentStartMonitor = GetStartMenuMonitor();
	}
	else
	{
		m_CurrentStartMonitor = nullptr;
	}

	for (const Window window : Window::FindEnum())
	{
		InsertWindow(window);
	}
}

void TaskbarAttributeWorker::InsertWindow(Window window)
{
	const HMONITOR monitor = window.monitor();

	// Note: find() is done here after the checks because if not we get some weird crashes when changing DPI.
	if ((WindowHelper::IsUserWindow(window) || m_Cfg.Whitelist.Matches(window)) && !m_Cfg.Blacklist.Matches(window))
	{
		if (window.maximised())
		{
			if (const auto iter = m_Taskbars.find(monitor); iter != m_Taskbars.end())
			{
				iter->second.MaximisedWindows.insert(window);
				iter->second.NormalWindows.erase(window);
			}

			return;
		}
		else if (!window.minimised())
		{
			if (const auto iter = m_Taskbars.find(monitor); iter != m_Taskbars.end())
			{
				iter->second.MaximisedWindows.erase(window);
				iter->second.NormalWindows.insert(window);
			}

			return;
		}
	}

	if (const auto iter = m_Taskbars.find(monitor); iter != m_Taskbars.end())
	{
		iter->second.MaximisedWindows.erase(window);
		iter->second.NormalWindows.erase(window);
	}
}

void TaskbarAttributeWorker::DumpWindowSet(std::wstring_view prefix, const std::unordered_set<Window> &set)
{
	if (!set.empty())
	{
		for (const Window window : set)
		{
			MessagePrint(spdlog::level::off, fmt::format(fmt(L"{}{} [{}] [{}] [{}]"), prefix, static_cast<void *>(window.handle()), window.title(), window.classname(), window.file().filename().native()));
		}
	}
	else
	{
		MessagePrint(spdlog::level::off, fmt::format(fmt(L"{}[none]"), prefix));
	}
}

void TaskbarAttributeWorker::ReturnToStock()
{
	ShowAeroPeekButton(m_Taskbars.at(m_MainTaskbarMonitor).TaskbarWindow, true);

	m_disableAttributeRefreshReply = true;

	for (const auto &[_, monInf] : m_Taskbars)
	{
		SetAttribute(monInf.TaskbarWindow, { ACCENT_NORMAL });
	}

	m_disableAttributeRefreshReply = false;
}

TaskbarAttributeWorker::TaskbarAttributeWorker(HINSTANCE hInstance, const Config &cfg) :
	MessageWindow(WORKER_WINDOW, WORKER_WINDOW, hInstance),
	m_PeekUnpeekHook(EVENT_SYSTEM_PEEKSTART, EVENT_SYSTEM_PEEKEND, std::bind(&TaskbarAttributeWorker::OnAeroPeekEnterExit, this, std::placeholders::_1)),
	m_CloakUncloakHook(EVENT_OBJECT_CLOAKED, EVENT_OBJECT_UNCLOAKED, BindEventHook(&TaskbarAttributeWorker::OnWindowStateChange)),
	m_MinimizeRestoreHook(EVENT_SYSTEM_MINIMIZESTART, EVENT_SYSTEM_MINIMIZEEND, BindEventHook(&TaskbarAttributeWorker::OnWindowStateChange)),
	m_ResizeMoveHook(EVENT_OBJECT_LOCATIONCHANGE, BindEventHook(&TaskbarAttributeWorker::OnWindowStateChange)),
	m_ShowHideHook(EVENT_OBJECT_SHOW, EVENT_OBJECT_HIDE, BindEventHook(&TaskbarAttributeWorker::OnWindowStateChange)),
	m_CreateDestroyHook(EVENT_OBJECT_CREATE, EVENT_OBJECT_DESTROY, BindEventHook(&TaskbarAttributeWorker::OnWindowCreateDestroy)),
	m_ForegroundChangeHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, BindEventHook(&TaskbarAttributeWorker::OnForegroundWindowChange)),
	m_PeekActive(false),
	m_disableAttributeRefreshReply(false),
	m_CurrentStartMonitor(nullptr),
	m_MainTaskbarMonitor(nullptr),
	m_IAV(wil::CoCreateInstance<AppVisibility, IAppVisibility>()),
	m_Cfg(cfg)
{
	const auto av_sink = winrt::make<AppVisibilitySink>(std::bind(&TaskbarAttributeWorker::OnStartVisibilityChange, this, std::placeholders::_1));

	m_IAVECookie.associate(m_IAV.get());
	HresultHandle(m_IAV->Advise(av_sink.get(), m_IAVECookie.put()), spdlog::level::warn, L"Failed to register app visibility sink.");

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

void TaskbarAttributeWorker::DumpState()
{
	MessagePrint(spdlog::level::off, L"===== Begin TaskbarAttributeWorker state dump =====");

	for (const auto &[monitor, info] : m_Taskbars)
	{
		MessagePrint(spdlog::level::off, fmt::format(fmt(L"Monitor {} [taskbar {}]:"), static_cast<void *>(monitor), static_cast<void *>(info.TaskbarWindow.handle())));

		MessagePrint(spdlog::level::off, L"    Maximised windows:");
		DumpWindowSet(L"        ", info.MaximisedWindows);

		MessagePrint(spdlog::level::off, L"    Normal windows:");
		DumpWindowSet(L"        ", info.NormalWindows);
	}

	MessagePrint(spdlog::level::off, fmt::format(fmt(L"User is using Aero Peek: {}"), m_PeekActive));
	MessagePrint(spdlog::level::off, fmt::format(fmt(L"Worker handles requests from hooks: {}"), !m_disableAttributeRefreshReply));

	if (m_CurrentStartMonitor != nullptr)
	{
		MessagePrint(spdlog::level::off, fmt::format(fmt(L"Start menu is opened on monitor: {}"), static_cast<void *>(m_CurrentStartMonitor)));
	}
	else
	{
		MessagePrint(spdlog::level::off, L"Start menu is opened: false");
	}

	MessagePrint(spdlog::level::off, fmt::format(fmt(L"Main taskbar is on monitor: {}"), static_cast<void *>(m_MainTaskbarMonitor)));

	MessagePrint(spdlog::level::off, L"Taskbars currently using normal appearance:");
	DumpWindowSet(L"    ", m_NormalTaskbars);

	MessagePrint(spdlog::level::off, L"===== End TaskbarAttributeWorker state dump =====");
}

void TaskbarAttributeWorker::ResetState()
{
	RefreshTaskbars();
	Poll();
	RefreshAeroPeekButton();
	for (const auto &[monitor, _] : m_Taskbars)
	{
		RefreshAttribute(monitor);
	}
}

TaskbarAttributeWorker::~TaskbarAttributeWorker()
{
	ReturnToStock();
}
