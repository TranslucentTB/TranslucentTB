#include "taskbarattributeworker.hpp"
#include "smart/boolguard.hpp"
#include "blacklist.hpp"
#include "constants.hpp"
#include "../ExplorerDetour/hook.hpp"
#include "ttberror.hpp"
#include "ttblog.hpp"
#include "win32.hpp"

const PFN_SET_WINDOW_COMPOSITION_ATTRIBUTE TaskbarAttributeWorker::SetWindowCompositionAttribute =
	reinterpret_cast<PFN_SET_WINDOW_COMPOSITION_ATTRIBUTE>(GetProcAddress(GetModuleHandle(L"user32.dll"), "SetWindowCompositionAttribute"));

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
			// Note: at() is done here after the check because if not we get some weird crashes when changing DPI.
			if (IsWindowMaximised(window))
			{
				m_Taskbars.at(monitor).MaximisedWindows.insert(window);
			}
			else
			{
				m_Taskbars.at(monitor).MaximisedWindows.erase(window);
			}

			if ((monitor == m_MainTaskbarMonitor && (Config::PEEK == Config::PEEK::DynamicMainMonitor || Config::PEEK == Config::PEEK::DynamicDesktopForeground)) ||
				Config::PEEK == Config::PEEK::DynamicAnyMonitor)
			{
				RefreshAeroPeekButton();
			}
			RefreshAttribute(monitor, true);
		}
	}
}

bool TaskbarAttributeWorker::IsWindowMaximised(Window window)
{
	return
		window.visible() &&
		window.state() == SW_MAXIMIZE &&
		!window.get_attribute<BOOL>(DWMWA_CLOAKED) &&
		window.on_current_desktop() &&
		!Blacklist::IsBlacklisted(window);
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
		if (Config::PEEK == Config::PEEK::DynamicDesktopForeground)
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
			OnWindowStateChange(true, EVENT_OBJECT_CREATE, window, idObject);
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
	if (Config::VERBOSE)
	{
		Log::OutputMessage(L"Refreshing taskbar handles.");
	}

	// Older handles are invalid, so clear the map to be ready for new ones
	m_Taskbars.clear();
	auto hooks = std::move(m_Hooks); // Keep old hooks alive while we rehook to keep the DLL loaded in Explorer. They will unhook automatically at the end of this function.
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

void TaskbarAttributeWorker::Poll()
{
	m_CurrentStartMonitor = nullptr;

	BOOL start_visible;
	if (ErrorHandle(m_IAV->IsLauncherVisible(&start_visible), Error::Level::Log, L"Failed to query launcher visibility state.") && start_visible)
	{
		m_CurrentStartMonitor = GetStartMenuMonitor();
	}

	for (const Window window : Window::FindEnum())
	{
		const HMONITOR monitor = window.monitor();
		if (m_Taskbars.contains(monitor))
		{
			if (IsWindowMaximised(window))
			{
				m_Taskbars.at(monitor).MaximisedWindows.insert(window);
			}
			else
			{
				m_Taskbars.at(monitor).MaximisedWindows.erase(window);
			}
		}
	}
}

bool TaskbarAttributeWorker::SetAttribute(Window window, Config::TASKBAR_APPEARANCE config)
{
	if (SetWindowCompositionAttribute)
	{
		if (config.ACCENT == ACCENT_NORMAL)
		{
			window.send_message(WM_THEMECHANGED);
			return false;
		}

		ACCENT_POLICY policy = {
			config.ACCENT,
			2,
			(config.COLOR & 0xFF00FF00) + ((config.COLOR & 0x00FF0000) >> 16) + ((config.COLOR & 0x000000FF) << 16),
			0
		};

		if (policy.AccentState == ACCENT_ENABLE_ACRYLICBLURBEHIND && policy.GradientColor >> 24 == 0x00)
		{
			// Fluent mode doesn't likes a completely 0 opacity
			policy.GradientColor = (0x01 << 24) + (policy.GradientColor & 0x00FFFFFF);
		}

		const WINDOWCOMPOSITIONATTRIBDATA data = {
			WCA_ACCENT_POLICY,
			&policy,
			sizeof(policy)
		};

		SetWindowCompositionAttribute(window, &data);
		return true;
	}
	else
	{
		return false;
	}
}

Config::TASKBAR_APPEARANCE TaskbarAttributeWorker::GetConfigForMonitor(HMONITOR monitor, bool skipCheck)
{
	if (Config::START_ENABLED && m_CurrentStartMonitor == monitor)
	{
		return Config::START_APPEARANCE;
	}

	if (Config::MAXIMISED_ENABLED)
	{
		if (Config::MAXIMISED_REGULAR_ON_PEEK && m_PeekActive)
		{
			return Config::REGULAR_APPEARANCE;
		}
		else if ((skipCheck || m_Taskbars.contains(monitor)) &&
			!m_Taskbars.at(monitor).MaximisedWindows.empty())
		{
			return Config::MAXIMISED_APPEARANCE;
		}
	}

	return Config::REGULAR_APPEARANCE;
}

bool TaskbarAttributeWorker::RefreshAttribute(HMONITOR monitor, bool skipCheck)
{
	if (skipCheck || m_Taskbars.contains(monitor))
	{
		return SetAttribute(m_Taskbars.at(monitor).TaskbarWindow, GetConfigForMonitor(monitor, true));
	}
	else
	{
		return false;
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

	switch (Config::PEEK)
	{
	case Config::PEEK::Enabled:
	case Config::PEEK::Disabled:
		ShowAeroPeekButton(taskbarInfo.TaskbarWindow, Config::PEEK == Config::PEEK::Enabled);
		break;

	case Config::PEEK::DynamicMainMonitor:
		ShowAeroPeekButton(taskbarInfo.TaskbarWindow, !taskbarInfo.MaximisedWindows.empty());
		break;

	case Config::PEEK::DynamicAnyMonitor:
		ShowAeroPeekButton(taskbarInfo.TaskbarWindow, std::any_of(m_Taskbars.begin(), m_Taskbars.end(), [](const auto &kvp)
		{
			return !kvp.second.MaximisedWindows.empty();
		}));
		break;

	case Config::PEEK::DynamicDesktopForeground:
	{
		// The desktop window has a child window with this class. The desktop window in
		// itself has no unique identifier however, it's just one of the many WorkerW windows.
		const Window foreground = Window::ForegroundWindow();
		bool isDesktop = false;

		if (foreground.monitor() == m_MainTaskbarMonitor)
		{
			isDesktop = foreground.find_child(L"SHELLDLL_DefView").valid();

			// Consider the taskbar as part of the desktop if there is no maximised window on the main monitor.
			// Consider being on the desktop if the foreground window is cloacked or invisible and there are no maximised windows.
			// Some apps such as Discord exhibit a weird behavior when being closed: the window just becomes invisible and it still is the foreground window.
			if (!isDesktop)
			{
				const auto &mainMonInfo = m_Taskbars.at(m_MainTaskbarMonitor);
				isDesktop = (foreground == mainMonInfo.TaskbarWindow || !foreground.visible() || foreground.get_attribute<BOOL>(DWMWA_CLOAKED)) && mainMonInfo.MaximisedWindows.empty();
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
	const Window window = reinterpret_cast<HWND>(lParam);
	if (m_Taskbars.contains(window.monitor()))
	{
		const auto taskbar = m_Taskbars.at(window.monitor()).TaskbarWindow;
		if (taskbar == window)
		{
			const auto config = GetConfigForMonitor(taskbar.monitor(), true);
			if (config.ACCENT == ACCENT_NORMAL || m_returningToStock)
			{
				return 0;
			}
			else
			{
				return RefreshAttribute(taskbar.monitor(), true);
			}
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return 0;
	}
}

void TaskbarAttributeWorker::ReturnToStock()
{
	ShowAeroPeekButton(m_Taskbars.at(m_MainTaskbarMonitor).TaskbarWindow, true);

	bool_guard guard(m_returningToStock);
	for (const auto &[_, monInf] : m_Taskbars)
	{
		SetAttribute(monInf.TaskbarWindow, { ACCENT_NORMAL });
	}
}

EventHook::callback_t TaskbarAttributeWorker::BindHook()
{
	return std::bind(&TaskbarAttributeWorker::OnWindowStateChange, this, false, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}

TaskbarAttributeWorker::TaskbarAttributeWorker(HINSTANCE hInstance) :
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
	m_returningToStock(false)
{
	const auto av_sink = winrt::make<AppVisibilitySink>(std::bind(&TaskbarAttributeWorker::OnStartVisibilityChange, this, std::placeholders::_1));

	m_IAVECookie.associate(m_IAV.get());
	ErrorHandle(m_IAV->Advise(av_sink.get(), m_IAVECookie.put()), Error::Level::Log, L"Failed to register app visibility sink.");

	RegisterCallback(Hook::RequestAttributeRefresh, std::bind(&TaskbarAttributeWorker::OnRequestAttributeRefresh, this, std::placeholders::_1, std::placeholders::_2));

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
