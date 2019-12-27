#include "taskbarattributeworker.hpp"
#include <member_thunk/member_thunk.hpp>

#include "constants.hpp"
#include "../ExplorerDetour/explorerdetour.hpp"
#include "../../ProgramLog/error/win32.hpp"
#include "undoc/winuser.hpp"
#include "util/fmt.hpp"
#include "win32.hpp"
#include "../windows/windowhelper.hpp"

const PFN_SET_WINDOW_COMPOSITION_ATTRIBUTE TaskbarAttributeWorker::SetWindowCompositionAttribute =
	reinterpret_cast<PFN_SET_WINDOW_COMPOSITION_ATTRIBUTE>(GetProcAddress(GetModuleHandle(SWCA_DLL), SWCA_ORDINAL));

void TaskbarAttributeWorker::OnAeroPeekEnterExit(DWORD event, HWND, LONG, LONG, DWORD, DWORD)
{
	m_PeekActive = event == EVENT_SYSTEM_PEEKSTART;
	if (m_Config.UseRegularAppearanceWhenPeeking)
	{
		for (auto iter = m_Taskbars.begin(); iter != m_Taskbars.end(); ++iter)
		{
			RefreshAttribute(iter);
		}
	}
}

void TaskbarAttributeWorker::OnWindowStateChange(DWORD, HWND hwnd, LONG idObject, LONG, DWORD, DWORD)
{
	if (Window window(hwnd); idObject == OBJID_WINDOW && window.valid())
	{
		const auto iter = InsertWindow(window);

		if (iter != m_Taskbars.end())
		{
			// RefreshAttribute done before because RefreshAeroPeekButton triggers
			// Windows internal message loop, see comment in InsertWindow.
			RefreshAttribute(iter);

			if ((iter->first == m_MainTaskbarMonitor && (m_Config.Peek == PeekBehavior::WindowMaximisedOnMainMonitor || m_Config.Peek == PeekBehavior::DesktopIsForegroundWindow)) ||
				m_Config.Peek == PeekBehavior::WindowMaximisedOnAnyMonitor)
			{
				RefreshAeroPeekButton();
			}
		}
	}
}

void TaskbarAttributeWorker::OnWindowCreateDestroy(DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD idEventThread, DWORD dwmsEventTime)
{
	if (Window window(hwnd); idObject == OBJID_WINDOW && window.valid())
	{
		if (const auto className = window.classname(); className == TASKBAR || className == SECONDARY_TASKBAR)
		{
			MessagePrint(spdlog::level::debug, L"A taskbar got created or destroyed, refreshing...");
			RefreshTaskbars();
		}
	}

	OnWindowStateChange(event, hwnd, idObject, idChild, idEventThread, dwmsEventTime);
}

void TaskbarAttributeWorker::OnForegroundWindowChange(DWORD, HWND hwnd, LONG idObject, LONG, DWORD, DWORD)
{
	if (Window window(hwnd); idObject == OBJID_WINDOW && window.valid() &&
		m_Config.Peek == PeekBehavior::DesktopIsForegroundWindow)
	{
		RefreshAeroPeekButton();
	}
}

void TaskbarAttributeWorker::OnStartVisibilityChange(bool state)
{
	const HMONITOR mon = state
		? m_CurrentStartMonitor = GetStartMenuMonitor()
		: std::exchange(m_CurrentStartMonitor, nullptr);

	if (const auto iter = m_Taskbars.find(mon); iter != m_Taskbars.end())
	{
		RefreshAttribute(iter);
	}

	const std::wstring_view msg = state
		? L"Start menu opened on monitor {}"
		: L"Start menu closed on monitor {}";
	Util::small_wmemory_buffer<50> buf;
	fmt::format_to(buf, msg, static_cast<void*>(mon));

	MessagePrint(spdlog::level::debug, buf);
}

LRESULT TaskbarAttributeWorker::OnRequestAttributeRefresh(LPARAM lParam)
{
	const Window window = reinterpret_cast<HWND>(lParam);
	if (const auto iter = m_Taskbars.find(window.monitor()); iter != m_Taskbars.end() && iter->second.TaskbarWindow == window)
	{
		if (const auto config = GetConfig(iter); config.Accent != ACCENT_NORMAL)
		{
			SetAttribute(iter->second.TaskbarWindow, config);
			return 1;
		}
	}

	return 0;
}

LRESULT TaskbarAttributeWorker::MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == m_TaskbarCreatedMessage || uMsg == WM_DISPLAYCHANGE)
	{
		ResetState();
		return 0;
	}
	else if (uMsg == m_RefreshRequestedMessage)
	{
		if (!m_disableAttributeRefreshReply)
		{
			return OnRequestAttributeRefresh(lParam);
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return MessageWindow::MessageHandler(uMsg, wParam, lParam);
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

	switch (m_Config.Peek)
	{
	case PeekBehavior::AlwaysShow:
	case PeekBehavior::AlwaysHide:
		ShowAeroPeekButton(taskbarInfo.TaskbarWindow, m_Config.Peek == PeekBehavior::AlwaysShow);
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

TaskbarAppearance TaskbarAttributeWorker::GetConfig(taskbar_iterator taskbar) const
{
	if (m_Config.UseRegularAppearanceWhenPeeking && m_PeekActive)
	{
		return m_Config.DesktopAppearance;
	}

	if (m_Config.StartOpenedAppearance.Enabled && m_CurrentStartMonitor == taskbar->first)
	{
		return m_Config.StartOpenedAppearance;
	}

	if (m_Config.MaximisedWindowAppearance.Enabled)
	{
		if (!taskbar->second.MaximisedWindows.empty())
		{
			return m_Config.MaximisedWindowAppearance;
		}
	}

	if (m_Config.VisibleWindowAppearance.Enabled)
	{
		if (!taskbar->second.MaximisedWindows.empty() || !taskbar->second.NormalWindows.empty())
		{
			return m_Config.VisibleWindowAppearance;
		}
	}

	return m_Config.DesktopAppearance;
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

void TaskbarAttributeWorker::RefreshAttribute(taskbar_iterator taskbar)
{
	SetAttribute(taskbar->second.TaskbarWindow, GetConfig(taskbar));
}

void TaskbarAttributeWorker::Poll()
{
	// TODO: check if user is using areo peek here (if not possible, assume they aren't)
	if (IsStartMenuOpened())
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
	InsertTaskbar(m_MainTaskbarMonitor = main_taskbar.monitor(), main_taskbar);

	for (const Window secondtaskbar : Window::FindEnum(SECONDARY_TASKBAR))
	{
		InsertTaskbar(secondtaskbar.monitor(), secondtaskbar);
	}
}

void TaskbarAttributeWorker::InsertTaskbar(HMONITOR mon, Window window)
{
	m_Taskbars.insert_or_assign(mon, MonitorInfo{ window });

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

TaskbarAttributeWorker::taskbar_iterator TaskbarAttributeWorker::InsertWindow(Window window)
{
	// Note: find() is done here after the checks because
	// some methods (most notably Window::on_current_desktop)
	// will trigger a Windows internal message loop,
	// which pumps messages to the worker. When the DPI is
	// changing, it means m_Taskbars is cleared while we still
	// have an iterator to it. Acquiring the iterator after the
	// call to on_current_desktop resolves this issue.
	bool windowMatches = (m_Config.Whitelist.IsFiltered(window) || WindowHelper::IsUserWindow(window)) && !m_Config.Blacklist.IsFiltered(window);

	const auto iter = m_Taskbars.find(window.monitor());
	if (iter != m_Taskbars.end())
	{
		if (windowMatches && window.maximised())
		{
			iter->second.MaximisedWindows.insert(window);
			iter->second.NormalWindows.erase(window);
		}
		else if (windowMatches && !window.minimised())
		{
			iter->second.MaximisedWindows.erase(window);
			iter->second.NormalWindows.insert(window);
		}
		else
		{
			iter->second.MaximisedWindows.erase(window);
			iter->second.NormalWindows.erase(window);
		}
	}

	return iter;
}

void TaskbarAttributeWorker::DumpWindowSet(std::wstring_view prefix, const std::unordered_set<Window> &set)
{
	fmt::wmemory_buffer buf;
	if (!set.empty())
	{
		for (const Window window : set)
		{
			buf.clear();
			fmt::format_to(buf, fmt(L"{}{} [{}] [{}] [{}]"), prefix, static_cast<void*>(window.handle()), window.title(), window.classname(), window.file().filename().native());
			MessagePrint(spdlog::level::off, buf);
		}
	}
	else
	{
		fmt::format_to(buf, fmt(L"{}[none]"), prefix);
		MessagePrint(spdlog::level::off, buf);
	}
}

TaskbarAttributeWorker::hook_thunk TaskbarAttributeWorker::CreateThunk(void(TaskbarAttributeWorker:: *proc)(DWORD, HWND, LONG, LONG, DWORD, DWORD)) try
{
	return member_thunk::make(this, proc);
}
StdSystemErrorCatch(spdlog::level::critical, L"Failed to create worker member thunk!");

wil::unique_hwineventhook TaskbarAttributeWorker::CreateHook(DWORD eventMin, DWORD eventMax, const hook_thunk &thunk)
{
	wil::unique_hwineventhook hook(SetWinEventHook(eventMin, eventMax, nullptr, thunk->get_thunked_function(), 0, 0, WINEVENT_OUTOFCONTEXT));
	if (!hook)
	{
		MessagePrint(spdlog::level::warn, L"Failed to create a Windows event hook.");
	}
	return hook;
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

bool TaskbarAttributeWorker::IsStartMenuOpened()
{
	BOOL start_visible;
	const HRESULT hr = m_IAV->IsLauncherVisible(&start_visible);
	if (SUCCEEDED(hr))
	{
		return start_visible;
	}
	else
	{
		HresultHandle(hr, spdlog::level::info, L"Failed to query launcher visibility state.");
		return false;
	}
}

TaskbarAttributeWorker::TaskbarAttributeWorker(const Config &cfg, HINSTANCE hInstance) :
	MessageWindow(WORKER_WINDOW, WORKER_WINDOW, hInstance),
	m_PeekActive(false),
	m_disableAttributeRefreshReply(false),
	m_CurrentStartMonitor(nullptr),
	m_MainTaskbarMonitor(nullptr),
	m_Config(cfg),
	m_AeroPeekEnterExitThunk(CreateThunk(&TaskbarAttributeWorker::OnAeroPeekEnterExit)),
	m_WindowStateChangeThunk(CreateThunk(&TaskbarAttributeWorker::OnWindowStateChange)),
	m_WindowCreateDestroyThunk(CreateThunk(&TaskbarAttributeWorker::OnWindowCreateDestroy)),
	m_ForegroundWindowChangeThunk(CreateThunk(&TaskbarAttributeWorker::OnForegroundWindowChange)),
	m_PeekUnpeekHook(CreateHook(EVENT_SYSTEM_PEEKSTART, EVENT_SYSTEM_PEEKEND, m_AeroPeekEnterExitThunk)),
	m_CloakUncloakHook(CreateHook(EVENT_OBJECT_CLOAKED, EVENT_OBJECT_UNCLOAKED, m_WindowStateChangeThunk)),
	m_MinimizeRestoreHook(CreateHook(EVENT_SYSTEM_MINIMIZESTART, EVENT_SYSTEM_MINIMIZEEND, m_WindowStateChangeThunk)),
	m_ResizeMoveHook(CreateHook(EVENT_OBJECT_LOCATIONCHANGE, m_WindowStateChangeThunk)),
	m_ShowHideHook(CreateHook(EVENT_OBJECT_SHOW, EVENT_OBJECT_HIDE, m_WindowStateChangeThunk)),
	m_CreateDestroyHook(CreateHook(EVENT_OBJECT_CREATE, EVENT_OBJECT_DESTROY, m_WindowCreateDestroyThunk)),
	m_ForegroundChangeHook(CreateHook(EVENT_SYSTEM_FOREGROUND, m_ForegroundWindowChangeThunk)),
	m_TitleChangeHook(CreateHook(EVENT_OBJECT_NAMECHANGE, m_WindowStateChangeThunk))
{
	try
	{
		m_IAV = wil::CoCreateInstance<IAppVisibility>(CLSID_AppVisibility);
		const auto av_sink = winrt::make_self<AppVisibilitySink>();

		m_AVSinkRevoker = av_sink->StartOpened(winrt::auto_revoke, { this, &TaskbarAttributeWorker::OnStartVisibilityChange });

		m_IAVECookie.associate(m_IAV.get());
		HresultVerify(m_IAV->Advise(av_sink.get(), m_IAVECookie.put()), spdlog::level::warn, L"Failed to register app visibility sink.");
	}
	ResultExceptionCatch(spdlog::level::warn, L"Failed to create app visibility instance.");

	m_TaskbarCreatedMessage = RegisterWindowMessage(WM_TASKBARCREATED);
	if (!m_TaskbarCreatedMessage)
	{
		LastErrorHandle(spdlog::level::warn, L"Failed to register taskbar created message.");
	}

	m_RefreshRequestedMessage = RegisterWindowMessage(WM_TTBHOOKREQUESTREFRESH);
	if (!m_RefreshRequestedMessage)
	{
		LastErrorHandle(spdlog::level::warn, L"Failed to register hook refresh request message.");
	}

	ResetState();
}

void TaskbarAttributeWorker::ConfigurationChanged()
{
	RefreshAeroPeekButton();
	for (auto iter = m_Taskbars.begin(); iter != m_Taskbars.end(); ++iter)
	{
		RefreshAttribute(iter);
	}
}

void TaskbarAttributeWorker::DumpState()
{
	MessagePrint(spdlog::level::off, L"===== Begin TaskbarAttributeWorker state dump =====");

	Util::small_wmemory_buffer<60> buf;
	for (const auto &[monitor, info] : m_Taskbars)
	{
		buf.clear();
		fmt::format_to(buf, fmt(L"Monitor {} [taskbar {}]:"), static_cast<void*>(monitor), static_cast<void*>(info.TaskbarWindow.handle()));
		MessagePrint(spdlog::level::off, buf);

		MessagePrint(spdlog::level::off, L"    Maximised windows:");
		DumpWindowSet(L"        ", info.MaximisedWindows);

		MessagePrint(spdlog::level::off, L"    Normal windows:");
		DumpWindowSet(L"        ", info.NormalWindows);
	}

	buf.clear();
	fmt::format_to(buf, fmt(L"User is using Aero Peek: {}"), m_PeekActive);
	MessagePrint(spdlog::level::off, buf);

	buf.clear();
	fmt::format_to(buf, fmt(L"Worker handles requests from hooks: {}"), !m_disableAttributeRefreshReply);
	MessagePrint(spdlog::level::off, buf);

	if (m_CurrentStartMonitor != nullptr)
	{
		buf.clear();
		fmt::format_to(buf, fmt(L"Start menu is opened on monitor: {}"), static_cast<void*>(m_CurrentStartMonitor));
		MessagePrint(spdlog::level::off, buf);
	}
	else
	{
		MessagePrint(spdlog::level::off, L"Start menu is opened: false");
	}

	buf.clear();
	fmt::format_to(buf, fmt(L"Main taskbar is on monitor: {}"), static_cast<void*>(m_MainTaskbarMonitor));
	MessagePrint(spdlog::level::off, buf);

	MessagePrint(spdlog::level::off, L"Taskbars currently using normal appearance:");
	DumpWindowSet(L"    ", m_NormalTaskbars);

	MessagePrint(spdlog::level::off, L"===== End TaskbarAttributeWorker state dump =====");
}

void TaskbarAttributeWorker::ResetState()
{
	RefreshTaskbars();
	Poll();
	ConfigurationChanged();
}

TaskbarAttributeWorker::~TaskbarAttributeWorker()
{
	ReturnToStock();
}
