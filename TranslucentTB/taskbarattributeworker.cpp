#include "taskbarattributeworker.hpp"
#include "appvisibilitysink.hpp"
#include "../CPicker/boolguard.hpp"
#include "blacklist.hpp"
#include "common.hpp"
#include "createinstance.hpp"
#include "../ExplorerDetour/hook.hpp"
#include "ttberror.hpp"
#include "ttblog.hpp"
#include "win32.hpp"

void TaskbarAttributeWorker::OnWindowStateChange(DWORD, const Window &window, LONG idObject, ...)
{
	if (idObject == OBJID_WINDOW && window.valid())
	{
		HMONITOR monitor = window.monitor();
		if (m_Taskbars.count(monitor) != 0)
		{
			auto &monInf = m_Taskbars.at(monitor);
			if (IsWindowMaximised(window))
			{
				monInf.MaximisedWindows.insert(window);
			}
			else
			{
				monInf.MaximisedWindows.erase(window);
			}
		}

		RefreshAttribute(monitor);
	}
}

bool TaskbarAttributeWorker::IsWindowMaximised(const Window &window)
{
	return
		window.visible() &&
		window.state() == SW_MAXIMIZE &&
		!window.get_attribute<BOOL>(DWMWA_CLOAKED) &&
		window.on_current_desktop() &&
		!Blacklist::IsBlacklisted(window);
}

BOOL CALLBACK TaskbarAttributeWorker::EnumWindowsProcess(const HWND hWnd, const LPARAM lParam)
{
	auto pThis = reinterpret_cast<TaskbarAttributeWorker *>(lParam);
	const Window window(hWnd);

	HMONITOR monitor = window.monitor();
	if (IsWindowMaximised(window))
	{
		if (pThis->m_Taskbars.count(monitor) != 0)
		{
			pThis->m_Taskbars.at(monitor).MaximisedWindows.insert(window);
		}
	}
	else
	{
		if (pThis->m_Taskbars.count(monitor) != 0)
		{
			pThis->m_Taskbars.at(monitor).MaximisedWindows.erase(window);
		}
	}

	return true;
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
		HMONITOR old_start_mon = std::exchange(m_CurrentStartMonitor, nullptr);
		RefreshAttribute(old_start_mon);
	}
}

HMONITOR TaskbarAttributeWorker::GetStartMenuMonitor()
{
	return Window::ForegroundWindow().monitor();
}

void TaskbarAttributeWorker::RefreshTaskbars()
{
	if (Config::VERBOSE)
	{
		Log::OutputMessage(L"Refreshing taskbar handles.");
	}

	// Older handles are invalid, so clear the map to be ready for new ones
	m_Taskbars.clear();
	std::vector<TTBHook> hooks = std::move(m_Hooks); // Keep old hooks alive while we rehook to keep the DLL loaded in Explorer. They will unhook automatically at the end of this function.
	m_Hooks.clear(); // Bring back m_Hooks to a known state after being moved from.

	const Window main_taskbar = Window::Find(TASKBAR);
	m_Taskbars[main_taskbar.monitor()] = { main_taskbar };
	HookTaskbar(main_taskbar);

	for (const Window secondtaskbar : Window::FindEnum(SECONDARY_TASKBAR))
	{
		m_Taskbars[secondtaskbar.monitor()] = { secondtaskbar };
		HookTaskbar(secondtaskbar);
	}
}

void TaskbarAttributeWorker::HookTaskbar(const Window &window)
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

	EnumWindows(EnumWindowsProcess, reinterpret_cast<LPARAM>(this));
}

void TaskbarAttributeWorker::ResetState()
{
	RefreshTaskbars();
	Poll();
	for (const auto &[monitor, _] : m_Taskbars)
	{
		RefreshAttribute(monitor);
	}
}

bool TaskbarAttributeWorker::SetAttribute(const Window &window, const Config::TASKBAR_APPEARANCE &config)
{
	if (user32::SetWindowCompositionAttribute)
	{
		if (config.ACCENT == swca::ACCENT::ACCENT_NORMAL)
		{
			window.send_message(WM_THEMECHANGED);
			return false;
		}

		swca::ACCENTPOLICY policy = {
			config.ACCENT,
			2,
			(config.COLOR & 0xFF00FF00) + ((config.COLOR & 0x00FF0000) >> 16) + ((config.COLOR & 0x000000FF) << 16),
			0
		};

		if (policy.nAccentState == swca::ACCENT::ACCENT_ENABLE_FLUENT && policy.nColor >> 24 == 0x00)
		{
			// Fluent mode doesn't likes a completely 0 opacity
			policy.nColor = (0x01 << 24) + (policy.nColor & 0x00FFFFFF);
		}

		swca::WINCOMPATTRDATA data = {
			swca::WindowCompositionAttribute::WCA_ACCENT_POLICY,
			&policy,
			sizeof(policy)
		};

		user32::SetWindowCompositionAttribute(window, &data);
		return true;
	}
	else
	{
		return false;
	}
}

const Config::TASKBAR_APPEARANCE &TaskbarAttributeWorker::GetConfigForMonitor(HMONITOR monitor)
{
	if (m_Taskbars.count(monitor) != 0)
	{
		const auto &monInf = m_Taskbars.at(monitor);
		if (m_CurrentStartMonitor == monitor)
		{
			return Config::START_APPEARANCE;
		}
		else if (!monInf.MaximisedWindows.empty())
		{
			return Config::MAXIMISED_APPEARANCE;
		}
		else
		{
			return Config::REGULAR_APPEARANCE;
		}
	}

	return Config::REGULAR_APPEARANCE;
}

bool TaskbarAttributeWorker::RefreshAttribute(HMONITOR monitor)
{
	if (m_Taskbars.count(monitor) != 0)
	{
		return SetAttribute(m_Taskbars.at(monitor).TaskbarWindow, GetConfigForMonitor(monitor));
	}
	else
	{
		return false;
	}
}

long TaskbarAttributeWorker::OnRequestAttributeRefresh(WPARAM, const LPARAM lParam)
{
	const Window window = reinterpret_cast<HWND>(lParam);
	if (m_Taskbars.count(window.monitor()) != 0)
	{
		const auto &taskbar = m_Taskbars.at(window.monitor()).TaskbarWindow;
		if (taskbar == window)
		{
			const auto &config = GetConfigForMonitor(taskbar.monitor());
			if (config.ACCENT == swca::ACCENT::ACCENT_NORMAL || m_returningToStock)
			{
				return 0;
			}
			else
			{
				return RefreshAttribute(taskbar.monitor());
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

EventHook::callback_t TaskbarAttributeWorker::BindHook()
{
	return std::bind(&TaskbarAttributeWorker::OnWindowStateChange, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}

TaskbarAttributeWorker::TaskbarAttributeWorker(const HINSTANCE &hInstance) :
	MessageWindow(WORKER_WINDOW, WORKER_WINDOW, hInstance),
	m_CloackedHook(EVENT_OBJECT_CLOAKED, EVENT_OBJECT_CLOAKED, BindHook(), WINEVENT_OUTOFCONTEXT),
	m_UncloackedHook(EVENT_OBJECT_UNCLOAKED, EVENT_OBJECT_UNCLOAKED, BindHook(), WINEVENT_OUTOFCONTEXT),
	m_CreatedHook(EVENT_OBJECT_CREATE, EVENT_OBJECT_CREATE, BindHook(), WINEVENT_OUTOFCONTEXT),
	m_DestroyedHook(EVENT_OBJECT_DESTROY, EVENT_OBJECT_DESTROY, BindHook(), WINEVENT_OUTOFCONTEXT),
	m_MinimizedHook(EVENT_SYSTEM_MINIMIZESTART, EVENT_SYSTEM_MINIMIZESTART, BindHook(), WINEVENT_OUTOFCONTEXT),
	m_UnminimizedHook(EVENT_SYSTEM_MINIMIZEEND, EVENT_SYSTEM_MINIMIZEEND, BindHook(), WINEVENT_OUTOFCONTEXT),
	m_ResizeMoveHook(EVENT_OBJECT_LOCATIONCHANGE, EVENT_OBJECT_LOCATIONCHANGE, BindHook(), WINEVENT_OUTOFCONTEXT),
	m_CurrentStartMonitor(nullptr),
	m_IAV(create_instance<IAppVisibility>(CLSID_AppVisibility)),
	m_IAVECookie(0),
	m_returningToStock(false)
{
	if (m_IAV)
	{
		using namespace Microsoft::WRL; // See comment on AppVisibilitySink for reason why WRL is used here
		ComPtr<IAppVisibilityEvents> av_sink = Make<AppVisibilitySink>(std::bind(&TaskbarAttributeWorker::OnStartVisibilityChange, this, std::placeholders::_1));
		ErrorHandle(m_IAV->Advise(av_sink.Get(), &m_IAVECookie), Error::Level::Log, L"Failed to register app visibility sink.");
	}

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

void TaskbarAttributeWorker::ReturnToStock()
{
	bool_guard guard(m_returningToStock);
	for (const auto &[_, monInf] : m_Taskbars)
	{
		SetAttribute(monInf.TaskbarWindow, { swca::ACCENT::ACCENT_NORMAL });
	}
}

TaskbarAttributeWorker::~TaskbarAttributeWorker()
{
	if (m_IAVECookie)
	{
		ErrorHandle(m_IAV->Unadvise(m_IAVECookie), Error::Level::Log, L"Failed to unregister app visibility sink");
	}
}
