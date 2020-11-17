#include "taskbarattributeworker.hpp"
#include <algorithm>
#include <functional>
#include <member_thunk/member_thunk.hpp>

#include "constants.hpp"
#include "../ExplorerDetour/explorerdetour.hpp"
#include "../../ProgramLog/error/win32.hpp"
#include "undoc/dynamicloader.hpp"
#include "undoc/winuser.hpp"
#include "util/fmt.hpp"
#include "win32.hpp"

void TaskbarAttributeWorker::OnAeroPeekEnterExit(DWORD event, HWND, LONG, LONG, DWORD, DWORD)
{
	m_PeekActive = event == EVENT_SYSTEM_PEEKSTART;
	RefreshAllAttributes();
}

void TaskbarAttributeWorker::OnWindowStateChange(DWORD, HWND hwnd, LONG idObject, LONG, DWORD, DWORD)
{
	if (Window window(hwnd); idObject == OBJID_WINDOW && window.valid())
	{
		InsertWindow(window, true);
	}
}

void TaskbarAttributeWorker::OnWindowCreateDestroy(DWORD event, HWND hwnd, LONG idObject, LONG, DWORD, DWORD)
{
	if (const Window window(hwnd); idObject == OBJID_WINDOW)
	{
		if (event == EVENT_OBJECT_CREATE && window.valid())
		{
			if (const auto className = window.classname(); className && (*className == TASKBAR || *className == SECONDARY_TASKBAR))
			{
				MessagePrint(spdlog::level::debug, L"A taskbar got created, refreshing...");
				ResetState();
			}
			else
			{
				InsertWindow(window, true);
			}
		}
		else if (event == EVENT_OBJECT_DESTROY)
		{
			// events are asynchronous, the window might be invalid already
			// important to not try to query its info here, just go off the handle
			AttributeRefresher refresher(*this);
			for (auto it = m_Taskbars.begin(); it != m_Taskbars.end(); ++it)
			{
				if (it->second.TaskbarWindow == window)
				{
					MessagePrint(spdlog::level::debug, L"A taskbar got destroyed, refreshing...");
					ResetState();

					// iterators invalid
					refresher.disarm();
					return;
				}
				else if (it->second.MaximisedWindows.erase(window) > 0 || it->second.NormalWindows.erase(window) > 0)
				{
					refresher.refresh(it);
				}
			}
		}
	}
}

void TaskbarAttributeWorker::OnForegroundWindowChange(DWORD, HWND hwnd, LONG idObject, LONG, DWORD, DWORD)
{
	if (Window window(hwnd); idObject == OBJID_WINDOW && window.valid())
	{
		// placeholder
	}
}

void TaskbarAttributeWorker::OnStartVisibilityChange(bool state)
{
	HMONITOR mon = nullptr;
	if (state)
	{
		mon = m_CurrentStartMonitor = GetStartMenuMonitor();
	}
	else
	{
		mon = std::exchange(m_CurrentStartMonitor, nullptr);
	}

	if (const auto iter = m_Taskbars.find(mon); iter != m_Taskbars.end())
	{
		RefreshAttribute(iter);
	}

	if (Error::ShouldLog(spdlog::level::debug))
	{
		Util::small_wmemory_buffer<50> buf;
		fmt::format_to(buf, FMT_STRING(L"Start menu {} on monitor {}"), state ? L"opened" : L"closed", static_cast<void *>(mon));

		MessagePrint(spdlog::level::debug, buf);
	}
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
		MessagePrint(spdlog::level::debug, uMsg == WM_DISPLAYCHANGE ? L"Monitor configuration change detected, refreshing..." : L"Main taskbar got created, refreshing...");

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

TaskbarAppearance TaskbarAttributeWorker::GetConfig(taskbar_iterator taskbar) const
{
	if (m_PeekActive)
	{
		return m_Config.DesktopAppearance;
	}

	if (m_Config.StartOpenedAppearance.Enabled && m_CurrentStartMonitor == taskbar->first)
	{
		return m_Config.StartOpenedAppearance;
	}

	if (m_Config.MaximisedWindowAppearance.Enabled)
	{
		if (!SetOnlyContainsValidWindows(taskbar->second.MaximisedWindows))
		{
			return m_Config.MaximisedWindowAppearance;
		}
	}

	if (m_Config.VisibleWindowAppearance.Enabled)
	{
		if (!SetOnlyContainsValidWindows(taskbar->second.MaximisedWindows) || !SetOnlyContainsValidWindows(taskbar->second.NormalWindows))
		{
			return m_Config.VisibleWindowAppearance;
		}
	}

	return m_Config.DesktopAppearance;
}

void TaskbarAttributeWorker::ShowAeroPeekButton(Window taskbar, bool show)
{
	// todo: make lazy. weirdly cpu intensive
	// missing all error handling
	const Window peek = taskbar.find_child(L"TrayNotifyWnd").find_child(L"TrayShowDesktopButtonWClass");

	if (show)
	{
		SetWindowLong(peek, GWL_EXSTYLE, GetWindowLong(peek, GWL_EXSTYLE) & ~WS_EX_LAYERED);
	}
	else
	{
		SetWindowLong(peek, GWL_EXSTYLE, GetWindowLong(peek, GWL_EXSTYLE) | WS_EX_LAYERED);

		// Non-zero alpha makes the button still interactible, even if practically invisible.
		SetLayeredWindowAttributes(peek, 0, 1, LWA_ALPHA);
	}
}

void TaskbarAttributeWorker::SetAttribute(Window window, TaskbarAppearance config)
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

	if (config.Accent == ACCENT_ENABLE_ACRYLICBLURBEHIND && config.Color.A == 0)
	{
		// Acrylic mode doesn't likes a completely 0 opacity
		config.Color.A = 1;
	}

	ACCENT_POLICY policy = {
		config.Accent,
		2,
		config.Color.ToABGR(),
		0
	};

	const WINDOWCOMPOSITIONATTRIBDATA data = {
		WCA_ACCENT_POLICY,
		&policy,
		sizeof(policy)
	};

	if (!SetWindowCompositionAttribute(window, &data))
	{
		LastErrorHandle(spdlog::level::warn, L"Failed to set window composition attribute");
	}
}

void TaskbarAttributeWorker::RefreshAttribute(taskbar_iterator taskbar)
{
	const auto &cfg = GetConfig(taskbar);
	SetAttribute(taskbar->second.TaskbarWindow, cfg);

	// ShowAeroPeekButton triggers Windows internal message loop,
	// do not pass any member of taskbar map by reference.
	// See comment in InsertWindow.
	if (taskbar->first == m_MainTaskbarMonitor && !m_PeekActive)
	{
		ShowAeroPeekButton(taskbar->second.TaskbarWindow, /*TODO: cfg.ShowPeek*/false);
	}
}

void TaskbarAttributeWorker::RefreshAllAttributes()
{
	AttributeRefresher refresher(*this);
	for (auto it = m_Taskbars.begin(); it != m_Taskbars.end(); ++it)
	{
		refresher.refresh(it);
	}
}

void TaskbarAttributeWorker::InsertWindow(Window window, bool refresh)
{
	AttributeRefresher attrRefresher(*this);
	const auto refresher = [&attrRefresher, refresh](taskbar_iterator it)
	{
		if (refresh)
		{
			attrRefresher.refresh(it);
		}
	};

	// Note: The checks are done before iterating because
	// some methods (most notably Window::on_current_desktop)
	// will trigger a Windows internal message loop,
	// which pumps messages to the worker. When the DPI is
	// changing, it means m_Taskbars is cleared while we still
	// have an iterator to it. Acquiring the iterator after the
	// call to on_current_desktop resolves this issue.
	const bool windowMatches = (window.is_user_window() || m_Config.Whitelist.IsFiltered(window)) && !m_Config.Blacklist.IsFiltered(window);
	const HMONITOR mon = window.monitor();

	for (auto it = m_Taskbars.begin(); it != m_Taskbars.end(); ++it)
	{
		auto &maximised = it->second.MaximisedWindows;
		auto &normal = it->second.NormalWindows;

		if (it->first == mon)
		{
			if (windowMatches && window.maximised())
			{
				maximised.insert(window);
				normal.erase(window);

				refresher(it);
				continue;
			}
			else if (windowMatches && !window.minimised())
			{
				maximised.erase(window);
				normal.insert(window);

				refresher(it);
				continue;
			}
		}

		if (maximised.erase(window) > 0 || normal.erase(window) > 0)
		{
			refresher(it);
		}
	}
}

bool TaskbarAttributeWorker::SetOnlyContainsValidWindows(std::unordered_set<Window> &set)
{
	std::erase_if(set, std::not_fn(&Window::valid));
	return set.empty();
}

void TaskbarAttributeWorker::DumpWindowSet(std::wstring_view prefix, const std::unordered_set<Window> &set, bool showInfo)
{
	fmt::wmemory_buffer buf;
	if (!set.empty())
	{
		for (const Window window : set)
		{
			buf.clear();
			if (showInfo)
			{
				std::wstring title, className, fileName;
				if (auto titleOpt = window.title())
				{
					title = std::move(*titleOpt);
					if (auto classNameOpt = window.classname())
					{
						className = std::move(*classNameOpt);
						if (const auto fileOpt = window.file())
						{
							fileName = fileOpt->filename().native();
						}
					}
				}
				fmt::format_to(buf, FMT_STRING(L"{}{} [{}] [{}] [{}]"), prefix, static_cast<void *>(window.handle()), title, className, fileName);
			}
			else
			{
				fmt::format_to(buf, FMT_STRING(L"{}{}"), prefix, static_cast<void *>(window.handle()));
			}
			MessagePrint(spdlog::level::off, buf);
		}
	}
	else
	{
		fmt::format_to(buf, FMT_STRING(L"{}[none]"), prefix);
		MessagePrint(spdlog::level::off, buf);
	}
}

void TaskbarAttributeWorker::CreateAppVisibility()
{
	try
	{
		m_IAV = wil::CoCreateInstance<IAppVisibility>(CLSID_AppVisibility);
	}
	catch (const wil::ResultException &err)
	{
		ResultExceptionHandle(err, spdlog::level::warn, L"Failed to create app visibility instance.");
		return;
	}

	const auto av_sink = winrt::make_self<LauncherVisibilitySink<&TaskbarAttributeWorker::OnStartVisibilityChange, TaskbarAttributeWorker>>(this);
	m_IAVECookie.associate(m_IAV.get());
	HresultVerify(m_IAV->Advise(av_sink.get(), m_IAVECookie.put()), spdlog::level::warn, L"Failed to register app visibility sink.");
}

WINEVENTPROC TaskbarAttributeWorker::CreateThunk(void(CALLBACK TaskbarAttributeWorker:: *proc)(DWORD, HWND, LONG, LONG, DWORD, DWORD))
{
	return m_ThunkPage.make_thunk<WINEVENTPROC>(this, proc);
}

wil::unique_hwineventhook TaskbarAttributeWorker::CreateHook(DWORD eventMin, DWORD eventMax, WINEVENTPROC proc)
{
	if (wil::unique_hwineventhook hook { SetWinEventHook(eventMin, eventMax, nullptr, proc, 0, 0, WINEVENT_OUTOFCONTEXT) })
	{
		return hook;
	}
	else
	{
		MessagePrint(spdlog::level::critical, L"Failed to create a Windows event hook.");
	}
}

void TaskbarAttributeWorker::ReturnToStock()
{
	m_disableAttributeRefreshReply = true;
	auto guard = wil::scope_exit([this]
	{
		m_disableAttributeRefreshReply = false;
	});

	auto mainMonIt = m_Taskbars.end();
	for (auto it = m_Taskbars.begin(); it != m_Taskbars.end(); ++it)
	{
		SetAttribute(it->second.TaskbarWindow, { ACCENT_NORMAL });

		if (it->first == m_MainTaskbarMonitor)
		{
			mainMonIt = it;
		}
	}

	guard.reset();

	// Always do it last, in case of Windows internal message loop
	if (mainMonIt != m_Taskbars.end())
	{
		ShowAeroPeekButton(mainMonIt->second.TaskbarWindow, true);
	}
}

bool TaskbarAttributeWorker::IsStartMenuOpened()
{
	if (m_IAV)
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
		}
	}

	return false;
}

void TaskbarAttributeWorker::InsertTaskbar(HMONITOR mon, Window window)
{
	m_Taskbars.insert_or_assign(mon, MonitorInfo { window });

	if (auto hook = ExplorerDetour::Inject(window))
	{
		m_Hooks.push_back(std::move(hook));
	}
	else
	{
		LastErrorHandle(spdlog::level::critical, L"Failed to set hook.");
	}
}

TaskbarAttributeWorker::TaskbarAttributeWorker(const Config &cfg, HINSTANCE hInstance) :
	MessageWindow(WORKER_WINDOW, WORKER_WINDOW, hInstance),
	SetWindowCompositionAttribute(DynamicLoader::SetWindowCompositionAttribute()),
	m_PeekActive(false),
	m_disableAttributeRefreshReply(false),
	m_CurrentStartMonitor(nullptr),
	m_MainTaskbarMonitor(nullptr),
	m_Config(cfg),
	m_ThunkPage(member_thunk::allocate_page()),
	m_PeekUnpeekHook(CreateHook(EVENT_SYSTEM_PEEKSTART, EVENT_SYSTEM_PEEKEND, CreateThunk(&TaskbarAttributeWorker::OnAeroPeekEnterExit))),
	m_CreateDestroyHook(CreateHook(EVENT_OBJECT_CREATE, EVENT_OBJECT_DESTROY, CreateThunk(&TaskbarAttributeWorker::OnWindowCreateDestroy))),
	m_ForegroundChangeHook(CreateHook(EVENT_SYSTEM_FOREGROUND, CreateThunk(&TaskbarAttributeWorker::OnForegroundWindowChange))),
	m_TaskbarCreatedMessage(Window::RegisterMessage(WM_TASKBARCREATED)),
	m_RefreshRequestedMessage(Window::RegisterMessage(WM_TTBHOOKREQUESTREFRESH))
{
	const auto stateThunk = CreateThunk(&TaskbarAttributeWorker::OnWindowStateChange);
	m_CloakUncloakHook = CreateHook(EVENT_OBJECT_CLOAKED, EVENT_OBJECT_UNCLOAKED, stateThunk);
	m_MinimizeRestoreHook = CreateHook(EVENT_SYSTEM_MINIMIZESTART, EVENT_SYSTEM_MINIMIZEEND, stateThunk);
	m_ResizeMoveHook = CreateHook(EVENT_OBJECT_LOCATIONCHANGE, stateThunk);
	m_ShowHideHook = CreateHook(EVENT_OBJECT_SHOW, EVENT_OBJECT_HIDE, stateThunk);
	m_TitleChangeHook = CreateHook(EVENT_OBJECT_NAMECHANGE, stateThunk);

	m_ThunkPage.mark_executable();

	CreateAppVisibility();

	ResetState();
}

void TaskbarAttributeWorker::DumpState()
{
	MessagePrint(spdlog::level::off, L"===== Begin TaskbarAttributeWorker state dump =====");

	Util::small_wmemory_buffer<60> buf;
	for (const auto &[monitor, info] : m_Taskbars)
	{
		buf.clear();
		fmt::format_to(buf, FMT_STRING(L"Monitor {} [taskbar {}]:"), static_cast<void *>(monitor), static_cast<void *>(info.TaskbarWindow.handle()));
		MessagePrint(spdlog::level::off, buf);

		MessagePrint(spdlog::level::off, L"    Maximised windows:");
		DumpWindowSet(L"        ", info.MaximisedWindows);

		MessagePrint(spdlog::level::off, L"    Normal windows:");
		DumpWindowSet(L"        ", info.NormalWindows);
	}

	buf.clear();
	fmt::format_to(buf, FMT_STRING(L"User is using Aero Peek: {}"), m_PeekActive);
	MessagePrint(spdlog::level::off, buf);

	buf.clear();
	fmt::format_to(buf, FMT_STRING(L"Worker handles requests from hooks: {}"), !m_disableAttributeRefreshReply);
	MessagePrint(spdlog::level::off, buf);

	if (m_CurrentStartMonitor != nullptr)
	{
		buf.clear();
		fmt::format_to(buf, FMT_STRING(L"Start menu is opened on monitor: {}"), static_cast<void *>(m_CurrentStartMonitor));
		MessagePrint(spdlog::level::off, buf);
	}
	else
	{
		MessagePrint(spdlog::level::off, L"Start menu is opened: false");
	}

	buf.clear();
	fmt::format_to(buf, FMT_STRING(L"Main taskbar is on monitor: {}"), static_cast<void *>(m_MainTaskbarMonitor));
	MessagePrint(spdlog::level::off, buf);

	MessagePrint(spdlog::level::off, L"Taskbars currently using normal appearance:");
	DumpWindowSet(L"    ", m_NormalTaskbars, false);

	MessagePrint(spdlog::level::off, L"===== End TaskbarAttributeWorker state dump =====");
}

void TaskbarAttributeWorker::ResetState(bool rehook)
{
	MessagePrint(spdlog::level::debug, L"Resetting worker state");

	// Clear state
	m_PeekActive = false;
	m_CurrentStartMonitor = nullptr;
	m_MainTaskbarMonitor = nullptr;

	if (rehook)
	{
		m_Taskbars.clear();
		m_NormalTaskbars.clear();

		// Keep old hooks alive while we rehook to avoid DLL unload.
		auto oldHooks = std::move(m_Hooks);
		m_Hooks.clear();

		if (const Window main_taskbar = Window::Find(TASKBAR))
		{
			m_MainTaskbarMonitor = main_taskbar.monitor();
			InsertTaskbar(m_MainTaskbarMonitor, main_taskbar);
		}

		for (const Window secondtaskbar : Window::FindEnum(SECONDARY_TASKBAR))
		{
			InsertTaskbar(secondtaskbar.monitor(), secondtaskbar);
		}
	}
	else
	{
		for (auto &[mon, info] : m_Taskbars)
		{
			info.MaximisedWindows.clear();
			info.NormalWindows.clear();
		}
	}

	// This might race but it's not an issue because
	// it'll race on a single thread and only ends up
	// doing something twice.

	// TODO: check if aero peek is active

	if (IsStartMenuOpened())
	{
		m_CurrentStartMonitor = GetStartMenuMonitor();
	}

	for (const Window window : Window::FindEnum())
	{
		InsertWindow(window, false);
	}

	// Apply the calculated effects
	RefreshAllAttributes();
}

TaskbarAttributeWorker::~TaskbarAttributeWorker()
{
	ReturnToStock();
}
