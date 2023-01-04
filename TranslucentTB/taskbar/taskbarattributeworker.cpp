#include "taskbarattributeworker.hpp"
#include <functional>
#include <member_thunk/member_thunk.hpp>

#include "constants.hpp"
#include "../../ProgramLog/error/win32.hpp"
#include "../../ProgramLog/error/winrt.hpp"
#include "undoc/explorer.hpp"
#include "undoc/user32.hpp"
#include "undoc/winuser.hpp"
#include "win32.hpp"
#include "winrt/Windows.Foundation.h"

class TaskbarAttributeWorker::AttributeRefresher {
private:
	TaskbarAttributeWorker &m_Worker;
	taskbar_iterator m_MainMonIt;
	bool m_Refresh;

public:
	AttributeRefresher(TaskbarAttributeWorker &worker, bool refresh = true) noexcept :
		m_Worker(worker), m_MainMonIt(m_Worker.m_Taskbars.end()), m_Refresh(refresh) { }

	AttributeRefresher(const AttributeRefresher &) = delete;
	AttributeRefresher &operator =(const AttributeRefresher &) = delete;

	void refresh(taskbar_iterator it)
	{
		if (m_Refresh)
		{
			if (it->first == MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY))
			{
				assert(m_MainMonIt == m_Worker.m_Taskbars.end());
				m_MainMonIt = it;
			}
			else
			{
				m_Worker.RefreshAttribute(it);
			}
		}
	}

	void disarm() noexcept { m_MainMonIt = m_Worker.m_Taskbars.end(); }

	~AttributeRefresher() noexcept(false)
	{
		if (m_Refresh && m_MainMonIt != m_Worker.m_Taskbars.end())
		{
			m_Worker.RefreshAttribute(m_MainMonIt);
		}
	}
};

template<DWORD insert, DWORD remove>
void TaskbarAttributeWorker::WindowInsertRemove(DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD, DWORD)
{
	if (const Window window(hwnd); idObject == OBJID_WINDOW && idChild == CHILDID_SELF)
	{
		if (event == insert && window.valid())
		{
			InsertWindow(window, true);
		}
		else if (event == remove)
		{
			AttributeRefresher refresher(*this);
			for (auto it = m_Taskbars.begin(); it != m_Taskbars.end(); ++it)
			{
				RemoveWindow(window, it, refresher);
			}
		}
	}
}

void TaskbarAttributeWorker::OnAeroPeekEnterExit(DWORD event, HWND, LONG, LONG, DWORD, DWORD)
{
	m_PeekActive = event == EVENT_SYSTEM_PEEKSTART;
	MessagePrint(spdlog::level::debug, m_PeekActive ? L"Aero Peek entered" : L"Aero Peek exited");

	RefreshAllAttributes();
}

void TaskbarAttributeWorker::OnWindowStateChange(DWORD, HWND hwnd, LONG idObject, LONG idChild, DWORD, DWORD)
{
	if (const Window window(hwnd); idObject == OBJID_WINDOW && idChild == CHILDID_SELF && window.valid())
	{
		InsertWindow(window, true);
	}
}

void TaskbarAttributeWorker::OnWindowCreateDestroy(DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD, DWORD)
{
	if (const Window window(hwnd); idObject == OBJID_WINDOW && idChild == CHILDID_SELF)
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
				if (it->second.Taskbar.TaskbarWindow == window)
				{
					MessagePrint(spdlog::level::debug, L"A taskbar got destroyed, refreshing...");
					ResetState();

					// iterators invalid
					refresher.disarm();
					return;
				}

				RemoveWindow<LogWindowRemovalDestroyed>(window, it, refresher);
			}
		}
	}
}

void TaskbarAttributeWorker::OnForegroundWindowChange(DWORD, HWND hwnd, LONG idObject, LONG idChild, DWORD, DWORD)
{
	if (idObject == OBJID_WINDOW && idChild == CHILDID_SELF)
	{
		const Window oldForegroundWindow = std::exchange(m_ForegroundWindow, Window(hwnd).valid() ? hwnd : Window::NullWindow);

		if (Error::ShouldLog<spdlog::level::debug>())
		{
			MessagePrint(spdlog::level::debug, std::format(L"Changed foreground window to {}", DumpWindow(m_ForegroundWindow)));
		}

		AttributeRefresher refresher(*this);
		HMONITOR oldMonitor = nullptr;
		if (oldForegroundWindow)
		{
			oldMonitor = oldForegroundWindow.monitor();
			if (const auto it = m_Taskbars.find(oldMonitor); it != m_Taskbars.end())
			{
				refresher.refresh(it);
			}
		}

		if (m_ForegroundWindow)
		{
			if (auto newMonitor = m_ForegroundWindow.monitor(); newMonitor != oldMonitor)
			{
				if (const auto it = m_Taskbars.find(newMonitor); it != m_Taskbars.end())
				{
					refresher.refresh(it);
				}
			}
		}
	}
}

void TaskbarAttributeWorker::OnWindowOrderChange(DWORD, HWND hwnd, LONG idObject, LONG idChild, DWORD, DWORD)
{
	if (const Window window(hwnd); idObject == OBJID_WINDOW && idChild == CHILDID_SELF && window.valid())
	{
		if (const auto iter = m_Taskbars.find(window.monitor()); iter != m_Taskbars.end())
		{
			RefreshAttribute(iter);
		}
	}
}

void TaskbarAttributeWorker::OnStartVisibilityChange(bool state)
{
	HMONITOR mon = nullptr;
	if (state)
	{
		mon = m_CurrentStartMonitor = GetStartMenuMonitor();

		if (Error::ShouldLog<spdlog::level::debug>())
		{
			MessagePrint(spdlog::level::debug, std::format(L"Start menu opened on monitor {}", static_cast<void *>(mon)));
		}
	}
	else
	{
		mon = std::exchange(m_CurrentStartMonitor, nullptr);

		MessagePrint(spdlog::level::debug, L"Start menu closed");
	}

	if (const auto iter = m_Taskbars.find(mon); iter != m_Taskbars.end())
	{
		RefreshAttribute(iter);
	}
}

void TaskbarAttributeWorker::OnTaskViewVisibilityChange(bool state)
{
	m_TaskViewActive = state;
	MessagePrint(spdlog::level::debug, m_TaskViewActive ? L"Task View opened" : L"Task View closed");

	RefreshAllAttributes();
}

void TaskbarAttributeWorker::OnSearchVisibilityChange(bool state)
{
	HMONITOR mon = nullptr;
	if (state)
	{
		mon = m_CurrentSearchMonitor = GetSearchMonitor();

		if (Error::ShouldLog<spdlog::level::debug>())
		{
			MessagePrint(spdlog::level::debug, std::format(L"Search opened on monitor {}", static_cast<void *>(mon)));
		}
	}
	else
	{
		mon = std::exchange(m_CurrentSearchMonitor, nullptr);

		MessagePrint(spdlog::level::debug, L"Search closed");
	}

	if (const auto iter = m_Taskbars.find(mon); iter != m_Taskbars.end())
	{
		RefreshAttribute(iter);
	}
}

void TaskbarAttributeWorker::OnForceRefreshTaskbar(Window taskbar)
{
	m_disableAttributeRefreshReply = true;
	auto guard = wil::scope_exit([this]
	{
		m_disableAttributeRefreshReply = false;
	});

	taskbar.send_message(WM_DWMCOMPOSITIONCHANGED);
	guard.reset();
	taskbar.send_message(WM_DWMCOMPOSITIONCHANGED);
}

LRESULT TaskbarAttributeWorker::OnSystemSettingsChange(UINT uiAction, std::wstring_view)
{
	if (uiAction == SPI_SETWORKAREA)
	{
		MessagePrint(spdlog::level::debug, L"Work area change detected, refreshing...");
		ResetState();
	}

	return 0;
}

LRESULT TaskbarAttributeWorker::OnPowerBroadcast(const POWERBROADCAST_SETTING *settings)
{
	if (settings && settings->PowerSetting == GUID_POWER_SAVING_STATUS && settings->DataLength == sizeof(DWORD))
	{
		m_PowerSaver = *reinterpret_cast<const DWORD *>(&settings->Data);
		RefreshAllAttributes();
	}

	return TRUE;
}

LRESULT TaskbarAttributeWorker::OnRequestAttributeRefresh(LPARAM lParam)
{
	if (!m_disableAttributeRefreshReply)
	{
		const Window window = reinterpret_cast<HWND>(lParam);
		if (const auto iter = m_Taskbars.find(window.monitor()); iter != m_Taskbars.end() && iter->second.Taskbar.TaskbarWindow == window)
		{
			if (const auto config = GetConfig(iter); config.Accent != ACCENT_NORMAL)
			{
				SetAttribute(iter, config);
				return 1;
			}
		}
	}

	return 0;
}

LRESULT TaskbarAttributeWorker::MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_SETTINGCHANGE)
	{
		return OnSystemSettingsChange(static_cast<UINT>(wParam), lParam ? reinterpret_cast<const wchar_t *>(lParam) : std::wstring_view { });
	}
	else if (uMsg == WM_DISPLAYCHANGE)
	{
		MessagePrint(spdlog::level::debug, L"Monitor configuration change detected, refreshing...");
		ResetState();
		return 0;
	}
	else if (uMsg == WM_POWERBROADCAST && wParam == PBT_POWERSETTINGCHANGE)
	{
		return OnPowerBroadcast(reinterpret_cast<const POWERBROADCAST_SETTING *>(lParam));
	}
	else if (uMsg == m_TaskbarCreatedMessage)
	{
		MessagePrint(spdlog::level::debug, L"Main taskbar got created, refreshing...");
		ResetState();
		return 0;
	}
	else if (uMsg == m_RefreshRequestedMessage)
	{
		return OnRequestAttributeRefresh(lParam);
	}
	else if (uMsg == m_TaskViewVisibilityChangeMessage)
	{
		OnTaskViewVisibilityChange(wParam);
		return 0;
	}
	else if (uMsg == m_StartVisibilityChangeMessage)
	{
		OnStartVisibilityChange(wParam);
		return 0;
	}
	else if (uMsg == m_SearchVisibilityChangeMessage)
	{
		OnSearchVisibilityChange(wParam);
		return 0;
	}
	else if (uMsg == m_ForceRefreshTaskbar)
	{
		OnForceRefreshTaskbar(reinterpret_cast<HWND>(lParam));
		return 0;
	}

	return MessageWindow::MessageHandler(uMsg, wParam, lParam);
}

TaskbarAppearance TaskbarAttributeWorker::GetConfig(taskbar_iterator taskbar) const
{
	if (m_Config.BatterySaverAppearance.Enabled && m_PowerSaver)
	{
		return WithPreview(txmp::TaskbarState::BatterySaver, m_Config.BatterySaverAppearance);
	}

	if (m_Config.TaskViewOpenedAppearance.Enabled && m_TaskViewActive)
	{
		return WithPreview(txmp::TaskbarState::TaskViewOpened, m_Config.TaskViewOpenedAppearance);
	}

	// Task View is ignored by peek, so shall we
	if (m_PeekActive)
	{
		return WithPreview(txmp::TaskbarState::Desktop, m_Config.DesktopAppearance);
	}

	// on windows 11, search is considered open when start is, so we need to check for start first.
	if (m_Config.StartOpenedAppearance.Enabled)
	{
		bool startOpened;
		if (m_IsWindows11)
		{
			// checking the search monitor is more reliable on windows 11
			// so check the start monitor to see if it's open and then use the search monitor
			// to check *where* it's open.
			startOpened = m_CurrentStartMonitor != nullptr && m_CurrentSearchMonitor == taskbar->first;
		}
		else
		{
			startOpened = m_CurrentStartMonitor == taskbar->first;
		}

		if (startOpened)
		{
			return WithPreview(txmp::TaskbarState::StartOpened, m_Config.StartOpenedAppearance);
		}
	}

	if (m_Config.SearchOpenedAppearance.Enabled && m_CurrentSearchMonitor == taskbar->first)
	{
		return WithPreview(txmp::TaskbarState::SearchOpened, m_Config.SearchOpenedAppearance);
	}

	auto &maximisedWindows = taskbar->second.MaximisedWindows;
	const bool hasMaximizedWindows = SetContainsValidWindows(maximisedWindows);
	if (m_Config.MaximisedWindowAppearance.Enabled && hasMaximizedWindows)
	{
		if (m_Config.MaximisedWindowAppearance.HasRules())
		{
			for (const Window wnd : Window::DesktopWindow().get_ordered_childrens())
			{
				// find the highest maximized window in the z-order.
				if (maximisedWindows.contains(wnd))
				{
					if (const auto rule = m_Config.MaximisedWindowAppearance.FindRule(wnd))
					{
						// if it has a rule, use that rule
						return *rule;
					}
					else
					{
						// we only consider the highest z-order maximized window for rules
						// so stop looking through the z-order
						break;
					}
				}
			}
		}

		// otherwise, use the normal maximized state
		return WithPreview(txmp::TaskbarState::MaximisedWindow, m_Config.MaximisedWindowAppearance);
	}

	if (m_Config.VisibleWindowAppearance.Enabled && (hasMaximizedWindows || SetContainsValidWindows(taskbar->second.NormalWindows)))
	{
		// if there is no maximized window, and the foreground window is on the current monitor
		if (m_Config.VisibleWindowAppearance.HasRules() && !hasMaximizedWindows && m_ForegroundWindow.monitor() == taskbar->first)
		{
			// find a rule for the foreground window
			if (const auto rule = m_Config.VisibleWindowAppearance.FindRule(m_ForegroundWindow))
			{
				// if it has a rule, use that rule
				return *rule;
			}
		}

		// otherwise use normal visible state
		return WithPreview(txmp::TaskbarState::VisibleWindow, m_Config.VisibleWindowAppearance);
	}

	return WithPreview(txmp::TaskbarState::Desktop, m_Config.DesktopAppearance);
}

void TaskbarAttributeWorker::ShowAeroPeekButton(const TaskbarInfo &taskbar, bool show)
{
	if (const auto style = taskbar.PeekWindow.get_long_ptr(GWL_EXSTYLE))
	{
		const bool success = SetNewWindowExStyle(taskbar.PeekWindow, *style, show
			? (*style & ~WS_EX_LAYERED)
			: (*style | WS_EX_LAYERED));

		if (!show && success)
		{
			// Non-zero alpha makes the button still interactible, even if practically invisible.
			if (!SetLayeredWindowAttributes(taskbar.PeekWindow, 0, 1, LWA_ALPHA))
			{
				LastErrorHandle(spdlog::level::warn, L"Failed to set peek button layered attributes");
			}
		}
	}
}

void TaskbarAttributeWorker::ShowTaskbarLine(const TaskbarInfo &taskbar, bool show)
{
	if (auto workerW = taskbar.WorkerWWindow)
	{
		workerW.show(show ? SW_SHOWNA : SW_HIDE);
	}
	else if (taskbar.InnerXamlContent)
	{
		if (show)
		{
			if (auto rect = taskbar.InnerXamlContent.client_rect())
			{
				const float scaleFactor = static_cast<float>(GetDpiForWindow(taskbar.InnerXamlContent)) / USER_DEFAULT_SCREEN_DPI;

				// bottom taskbar is the only available option in Windows 11 22000.
				rect->top += std::lround(1.0f * scaleFactor);
				if (wil::unique_hrgn rgn { CreateRectRgnIndirect(&*rect) })
				{
					if (SetWindowRgn(taskbar.InnerXamlContent, rgn.get(), true))
					{
						rgn.release();
					}
					else
					{
						LastErrorHandle(spdlog::level::warn, L"Failed to set region of inner XAML window");
					}
				}
			}
		}
		else
		{
			if (!SetWindowRgn(taskbar.InnerXamlContent, nullptr, true)) [[unlikely]]
			{
				LastErrorHandle(spdlog::level::info, L"Failed to clear window region of inner taskbar XAML");
			}
		}
	}
}

void TaskbarAttributeWorker::SetAttribute(taskbar_iterator taskbar, TaskbarAppearance config)
{
	const auto window = taskbar->second.Taskbar.TaskbarWindow;

	if (config.Accent != ACCENT_NORMAL)
	{
		m_NormalTaskbars.erase(window);

		const bool isAcrylic = config.Accent == ACCENT_ENABLE_ACRYLICBLURBEHIND;
		if (isAcrylic && config.Color.A == 0)
		{
			// Acrylic mode doesn't likes a completely 0 opacity
			config.Color.A = 1;
		}

		ACCENT_POLICY policy = {
			config.Accent,
			static_cast<UINT>(isAcrylic ? 0 : 2),
			config.Color.ToABGR(),
			0
		};

		const WINDOWCOMPOSITIONATTRIBDATA data = {
			WCA_ACCENT_POLICY,
			&policy,
			sizeof(policy)
		};

		if (!SetWindowCompositionAttribute(window, &data)) [[unlikely]]
		{
			LastErrorHandle(spdlog::level::info, L"Failed to set window composition attribute");
		}
	}
	else if (const auto [it, inserted] = m_NormalTaskbars.insert(window); inserted)
	{
		// If this is in response to a window being moved, we send the message way too often
		// and Explorer doesn't like that too much.
		window.send_message(WM_DWMCOMPOSITIONCHANGED, 1, 0);
	}
}

void TaskbarAttributeWorker::RefreshAttribute(taskbar_iterator taskbar)
{
	const auto &cfg = GetConfig(taskbar);
	SetAttribute(taskbar, cfg);

	// These functions may trigger Windows internal message loops,
	// do not pass any member of taskbar map by reference.
	// See comment in InsertWindow.
	const auto taskbarInfo = taskbar->second.Taskbar;
	if (taskbarInfo.InnerXamlContent || taskbarInfo.WorkerWWindow)
	{
		ShowTaskbarLine(taskbarInfo, cfg.ShowLine);
	}
	else if (taskbarInfo.PeekWindow)
	{
		// Ignore changes when peek is active
		if (!m_PeekActive)
		{
			ShowAeroPeekButton(taskbarInfo, cfg.ShowPeek);
		}
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

void TaskbarAttributeWorker::LogWindowInsertion(const std::pair<std::unordered_set<Window>::iterator, bool> &result, std::wstring_view state, HMONITOR mon)
{
	if (result.second && Error::ShouldLog<spdlog::level::debug>())
	{
		MessagePrint(spdlog::level::debug, std::format(L"Inserting {} window {} to monitor {}", state, DumpWindow(*result.first), static_cast<void *>(mon)));
	}
}

void TaskbarAttributeWorker::LogWindowRemoval(std::wstring_view state, Window window, HMONITOR mon)
{
	if (Error::ShouldLog<spdlog::level::debug>())
	{
		MessagePrint(spdlog::level::debug, std::format(L"Removing {} window {} from monitor {}", state, DumpWindow(window), static_cast<void *>(mon)));
	}
}

void TaskbarAttributeWorker::LogWindowRemovalDestroyed(std::wstring_view state, Window window, HMONITOR mon)
{
	if (Error::ShouldLog<spdlog::level::debug>())
	{
		MessagePrint(spdlog::level::debug, std::format(L"Removing {} window {} [window destroyed] from monitor {}", state, static_cast<void *>(window.handle()), static_cast<void *>(mon)));
	}
}

void TaskbarAttributeWorker::InsertWindow(Window window, bool refresh)
{
	if (window.classname() == CORE_WINDOW) [[unlikely]]
	{
		// Windows.UI.Core.CoreWindow is always shell UI stuff
		// that we either have a dynamic mode for or should ignore.
		// so just skip it.
		return;
	}

	AttributeRefresher refresher(*this, refresh);

	// Note: The checks are done before iterating because
	// some methods (most notably Window::on_current_desktop)
	// will trigger a Windows internal message loop,
	// which pumps messages to the worker. When the DPI is
	// changing, it means m_Taskbars is cleared while we still
	// have an iterator to it. Acquiring the iterator after the
	// call to on_current_desktop resolves this issue.
	const bool windowMatches = window.is_user_window() && !m_Config.IgnoredWindows.IsFiltered(window);
	const HMONITOR mon = window.monitor();

	for (auto it = m_Taskbars.begin(); it != m_Taskbars.end(); ++it)
	{
		auto &maximised = it->second.MaximisedWindows;
		auto &normal = it->second.NormalWindows;

		if (it->first == mon)
		{
			if (windowMatches && window.maximised())
			{
				if (normal.erase(window) > 0)
				{
					LogWindowRemoval(L"normal", window, mon);
				}

				LogWindowInsertion(maximised.insert(window), L"maximised", mon);

				refresher.refresh(it);
				continue;
			}
			else if (windowMatches && !window.minimised())
			{
				if (maximised.erase(window) > 0)
				{
					LogWindowRemoval(L"maximised", window, mon);
				}

				LogWindowInsertion(normal.insert(window), L"normal", mon);

				refresher.refresh(it);
				continue;
			}

			// fall out the if if the window is minimized
		}

		RemoveWindow(window, it, refresher);
	}
}

template<void(*logger)(std::wstring_view, Window, HMONITOR)>
void TaskbarAttributeWorker::RemoveWindow(Window window, taskbar_iterator it, AttributeRefresher& refresher)
{
	bool erased = false;
	if (it->second.MaximisedWindows.erase(window) > 0)
	{
		logger(L"maximised", window, it->first);
		erased = true;
	}

	if (it->second.NormalWindows.erase(window) > 0)
	{
		logger(L"normal", window, it->first);
		erased = true;
	}

	// only refresh the taskbar once in the case the window is in both
	if (erased)
	{
		refresher.refresh(it);
	}
}

bool TaskbarAttributeWorker::SetNewWindowExStyle(Window wnd, LONG_PTR oldStyle, LONG_PTR newStyle)
{
	if (oldStyle != newStyle)
	{
		return wnd.set_long_ptr(GWL_EXSTYLE, newStyle).has_value();
	}
	else
	{
		return true;
	}
}

bool TaskbarAttributeWorker::SetContainsValidWindows(std::unordered_set<Window> &set)
{
	std::erase_if(set, [](Window wnd)
	{
		return !wnd.valid();
	});
	return !set.empty();
}

void TaskbarAttributeWorker::DumpWindowSet(std::wstring_view prefix, const std::unordered_set<Window> &set, bool showInfo)
{
	if (!set.empty())
	{
		std::wstring buf;
		for (const Window window : set)
		{
			buf.clear();
			if (showInfo)
			{
				buf += prefix;
				buf += DumpWindow(window);
			}
			else
			{
				std::format_to(std::back_inserter(buf), L"{}{}", prefix, static_cast<void *>(window.handle()));
			}
			MessagePrint(spdlog::level::off, buf);
		}
	}
	else
	{
		MessagePrint(spdlog::level::off, std::format(L"{}[none]", prefix));
	}
}

std::wstring TaskbarAttributeWorker::DumpWindow(Window window)
{
	if (window)
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

		return std::format(L"{} [{}] [{}] [{}]", static_cast<void *>(window.handle()), title, className, fileName);
	}
	else
	{
		return L"0x0";
	}
}

void TaskbarAttributeWorker::CreateAppVisibility()
{
	if (m_StartVisibilityChangeMessage)
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

		const auto av_sink = winrt::make_self<LauncherVisibilitySink>(*this, *m_StartVisibilityChangeMessage);
		m_IAVECookie.associate(m_IAV.get());
		HresultVerify(m_IAV->Advise(av_sink.get(), m_IAVECookie.put()), spdlog::level::warn, L"Failed to register app visibility sink.");
	}
}

void TaskbarAttributeWorker::CreateSearchManager()
{
	UnregisterSearchCallbacks();

	m_SearchManager = nullptr;
	m_SearchViewCoordinator = nullptr;

	if (m_SearchVisibilityChangeMessage)
	{
		if (m_IsWindows11)
		{
			try
			{
				using winrt::WindowsUdk::UI::Shell::ShellViewCoordinator;
				m_SearchViewCoordinator = ShellViewCoordinator { winrt::WindowsUdk::UI::Shell::ShellView::Search };

				m_SearchViewVisibilityChangedToken = m_SearchViewCoordinator.VisibilityChanged([this](const ShellViewCoordinator &coordinator, const wf::IInspectable &)
				{
					send_message(*m_SearchVisibilityChangeMessage, coordinator.Visibility() == winrt::WindowsUdk::UI::Shell::ViewVisibility::Visible);
				});
			}
			HresultErrorCatch(spdlog::level::warn, L"Failed to create ShellViewCoordinator");
		}
		else
		{
			try
			{
				using winrt::Windows::Internal::Shell::Experience::IShellExperienceManagerFactory;
				using winrt::Windows::Internal::Shell::Experience::ICortanaExperienceManager;

				auto serviceManager = wil::CoCreateInstance<IServiceProvider>(CLSID_ImmersiveShell, CLSCTX_LOCAL_SERVER);

				IShellExperienceManagerFactory factory(nullptr);
				winrt::check_hresult(serviceManager->QueryService(winrt::guid_of<IShellExperienceManagerFactory>(), winrt::guid_of<IShellExperienceManagerFactory>(), winrt::put_abi(factory)));

				m_SearchManager = factory.GetExperienceManager(win32::IsAtLeastBuild(19041) ? SEH_SearchApp : SEH_Cortana).as<ICortanaExperienceManager>();

				m_SuggestionsShownToken = m_SearchManager.SuggestionsShown([this](const ICortanaExperienceManager &, const wf::IInspectable &)
				{
					send_message(*m_SearchVisibilityChangeMessage, true);
				});

				m_SuggestionsHiddenToken = m_SearchManager.SuggestionsHidden([this](const ICortanaExperienceManager &, const wf::IInspectable &)
				{
					send_message(*m_SearchVisibilityChangeMessage, false);
				});
			}
			ResultExceptionCatch(spdlog::level::warn, L"Failed to create immersive shell service provider")
			HresultErrorCatch(spdlog::level::warn, L"Failed to query for ICortanaExperienceManager");
		}
	}
}

void TaskbarAttributeWorker::UnregisterSearchCallbacks() noexcept
{
	if (m_SearchManager)
	{
		if (m_SuggestionsShownToken)
		{
			m_SearchManager.SuggestionsShown(m_SuggestionsShownToken);
			m_SuggestionsShownToken = { };
		}

		if (m_SuggestionsHiddenToken)
		{
			m_SearchManager.SuggestionsHidden(m_SuggestionsHiddenToken);
			m_SuggestionsHiddenToken = { };
		}
	}

	if (m_SearchViewCoordinator)
	{
		if (m_SearchViewVisibilityChangedToken)
		{
			m_SearchViewCoordinator.VisibilityChanged(m_SearchViewVisibilityChangedToken);
			m_SearchViewVisibilityChangedToken = { };
		}
	}
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
	// Copy taskbars to a vector because some functions
	// trigger Windows internal message loop.
	std::vector<TaskbarInfo> taskbarInfos;
	for (auto it = m_Taskbars.begin(); it != m_Taskbars.end(); ++it)
	{
		SetAttribute(it, { ACCENT_NORMAL, { 0, 0, 0, 0 }, true, true });

		taskbarInfos.push_back(it->second.Taskbar);
	}

	for (const auto &taskbarInfo : taskbarInfos)
	{
		if (taskbarInfo.InnerXamlContent || taskbarInfo.WorkerWWindow)
		{
			ShowTaskbarLine(taskbarInfo, true);
		}
		else if (taskbarInfo.PeekWindow)
		{
			ShowAeroPeekButton(taskbarInfo, true);
		}
	}
}

bool TaskbarAttributeWorker::IsStartMenuOpened() const
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

bool TaskbarAttributeWorker::IsSearchOpened() const try
{
	if (m_SearchViewCoordinator)
	{
		return m_SearchViewCoordinator.Visibility() == winrt::WindowsUdk::UI::Shell::ViewVisibility::Visible;
	}
	else if (m_SearchManager)
	{
		return m_SearchManager.SuggestionsShowing();
	}
	else
	{
		return false;
	}
}
catch (const winrt::hresult_error &err)
{
	HresultErrorHandle(err, spdlog::level::info, L"Failed to check if search is opened");
	return false;
}

void TaskbarAttributeWorker::InsertTaskbar(HMONITOR mon, Window window)
{
	TaskbarInfo taskbarInfo = { .TaskbarWindow = window };
	if (m_IsWindows11)
	{
		if (win32::IsAtLeastBuild(22616))
		{
			taskbarInfo.WorkerWWindow = window.find_child(L"WorkerW");
		}
		else
		{
			taskbarInfo.InnerXamlContent = window.find_child(L"Windows.UI.Composition.DesktopWindowContentBridge", L"DesktopWindowXamlSource");
		}
	}
	else
	{
		taskbarInfo.PeekWindow = window.find_child(L"TrayNotifyWnd").find_child(L"TrayShowDesktopButtonWClass");
	}

	m_NormalTaskbars.insert(window);
	m_Taskbars.insert_or_assign(mon, MonitorInfo { taskbarInfo });

	if (wil::unique_hhook hook { m_InjectExplorerHook(window) })
	{
		m_Hooks.push_back(std::move(hook));
	}
	else
	{
		LastErrorHandle(spdlog::level::critical, L"Failed to set hook.");
	}
}

wil::unique_hmodule TaskbarAttributeWorker::LoadHookDll(const std::optional<std::filesystem::path> &storageFolder)
{
	const auto [loc, hr] = win32::GetExeLocation();
	HresultVerify(hr, spdlog::level::critical, L"Failed to determine executable location!");

	std::filesystem::path dllPath = loc.parent_path() / L"ExplorerHooks.dll";

	if (storageFolder)
	{
		// copy the file over to a place Explorer can read. It can't be injected from WindowsApps.
		auto tempDllPath = *storageFolder / L"TempState" / "ExplorerHooks.dll";

		std::error_code err;
		std::filesystem::copy_file(dllPath, tempDllPath, std::filesystem::copy_options::update_existing, err);
		if (err)
		{
			StdErrorCodeHandle(err, spdlog::level::critical, L"Failed to copy ExplorerHooks.dll");
		}

		dllPath = std::move(tempDllPath);
	}

	wil::unique_hmodule dll(LoadLibraryEx(dllPath.c_str(), nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32));
	if (!dll)
	{
		LastErrorHandle(spdlog::level::critical, L"Failed to load ExplorerHooks.dll");
	}

	return dll;
}

TaskbarAttributeWorker::TaskbarAttributeWorker(const Config &cfg, HINSTANCE hInstance, DynamicLoader &loader, const std::optional<std::filesystem::path> &storageFolder) :
	MessageWindow(TTB_WORKERWINDOW, TTB_WORKERWINDOW, hInstance, WS_POPUP, WS_EX_NOREDIRECTIONBITMAP),
	SetWindowCompositionAttribute(loader.SetWindowCompositionAttribute()),
	ShouldSystemUseDarkMode(loader.ShouldSystemUseDarkMode()),
	m_PowerSaver(false),
	m_TaskViewActive(false),
	m_PeekActive(false),
	m_disableAttributeRefreshReply(false),
	m_ResettingState(false),
	m_ResetStateReentered(false),
	m_CurrentStartMonitor(nullptr),
	m_CurrentSearchMonitor(nullptr),
	m_Config(cfg),
	m_ThunkPage(member_thunk::allocate_page()),
	m_PeekUnpeekHook(CreateHook(EVENT_SYSTEM_PEEKSTART, EVENT_SYSTEM_PEEKEND, CreateThunk(&TaskbarAttributeWorker::OnAeroPeekEnterExit))),
	m_CloakUncloakHook(CreateHook(EVENT_OBJECT_CLOAKED, EVENT_OBJECT_UNCLOAKED, CreateThunk(&TaskbarAttributeWorker::WindowInsertRemove<EVENT_OBJECT_UNCLOAKED, EVENT_OBJECT_CLOAKED>))),
	m_MinimizeRestoreHook(CreateHook(EVENT_SYSTEM_MINIMIZESTART, EVENT_SYSTEM_MINIMIZEEND, CreateThunk(&TaskbarAttributeWorker::WindowInsertRemove<EVENT_SYSTEM_MINIMIZEEND, EVENT_SYSTEM_MINIMIZESTART>))),
	m_ShowHideHook(CreateHook(EVENT_OBJECT_SHOW, EVENT_OBJECT_HIDE, CreateThunk(&TaskbarAttributeWorker::WindowInsertRemove<EVENT_OBJECT_SHOW, EVENT_OBJECT_HIDE>))),
	m_CreateDestroyHook(CreateHook(EVENT_OBJECT_CREATE, EVENT_OBJECT_DESTROY, CreateThunk(&TaskbarAttributeWorker::OnWindowCreateDestroy))),
	m_ForegroundChangeHook(CreateHook(EVENT_SYSTEM_FOREGROUND, CreateThunk(&TaskbarAttributeWorker::OnForegroundWindowChange))),
	m_OrderChangeHook(CreateHook(EVENT_OBJECT_REORDER, CreateThunk(&TaskbarAttributeWorker::OnWindowOrderChange))),
	m_SearchManager(nullptr),
	m_SearchViewCoordinator(nullptr),
	m_TaskbarCreatedMessage(Window::RegisterMessage(WM_TASKBARCREATED)),
	m_RefreshRequestedMessage(Window::RegisterMessage(WM_TTBHOOKREQUESTREFRESH)),
	m_TaskViewVisibilityChangeMessage(Window::RegisterMessage(WM_TTBHOOKTASKVIEWVISIBILITYCHANGE)),
	m_IsTaskViewOpenedMessage(Window::RegisterMessage(WM_TTBHOOKISTASKVIEWOPENED)),
	m_StartVisibilityChangeMessage(Window::RegisterMessage(WM_TTBSTARTVISIBILITYCHANGE)),
	m_SearchVisibilityChangeMessage(Window::RegisterMessage(WM_TTBSEARCHVISIBILITYCHANGE)),
	m_ForceRefreshTaskbar(Window::RegisterMessage(WM_TTBFORCEREFRESHTASKBAR)),
	m_LastExplorerPid(0),
	m_HookDll(LoadHookDll(storageFolder)),
	m_InjectExplorerHook(reinterpret_cast<PFN_INJECT_EXPLORER_HOOK>(GetProcAddress(m_HookDll.get(), "InjectExplorerHook"))),
	m_IsWindows11(win32::IsAtLeastBuild(22000))
{
	if (!m_InjectExplorerHook)
	{
		LastErrorHandle(spdlog::level::critical, L"Failed to get address of InjectExplorerHook");
	}

	const auto stateThunk = CreateThunk(&TaskbarAttributeWorker::OnWindowStateChange);
	m_ResizeMoveHook = CreateHook(EVENT_OBJECT_LOCATIONCHANGE, stateThunk);
	m_TitleChangeHook = CreateHook(EVENT_OBJECT_NAMECHANGE, stateThunk);
	m_ParentChangeHook = CreateHook(EVENT_OBJECT_PARENTCHANGE, stateThunk);
	m_ThunkPage.mark_executable();

	m_PowerSaverHook.reset(RegisterPowerSettingNotification(m_WindowHandle, &GUID_POWER_SAVING_STATUS, DEVICE_NOTIFY_WINDOW_HANDLE));
	if (!m_PowerSaverHook)
	{
		LastErrorHandle(spdlog::level::warn, L"Failed to create battery saver notification handle");
	}

	CreateAppVisibility();

	// we don't want to consider the first state reset as an Explorer restart.
	ResetState(true);
}

void TaskbarAttributeWorker::DumpState()
{
	MessagePrint(spdlog::level::off, L"===== Begin TaskbarAttributeWorker state dump =====");

	std::wstring buf;
	for (const auto &[monitor, info] : m_Taskbars)
	{
		buf.clear();
		std::format_to(std::back_inserter(buf), L"Monitor {}:", static_cast<void *>(monitor));
		MessagePrint(spdlog::level::off, buf);

		buf.clear();
		std::format_to(std::back_inserter(buf), L"\tTaskbar handle: {}", DumpWindow(info.Taskbar.TaskbarWindow));
		MessagePrint(spdlog::level::off, buf);

		buf.clear();
		std::format_to(std::back_inserter(buf), L"\tPeek button handle: {}", DumpWindow(info.Taskbar.PeekWindow));
		MessagePrint(spdlog::level::off, buf);

		buf.clear();
		std::format_to(std::back_inserter(buf), L"\tInner XAML handle: {}", DumpWindow(info.Taskbar.InnerXamlContent));
		MessagePrint(spdlog::level::off, buf);

		buf.clear();
		std::format_to(std::back_inserter(buf), L"\tWorkerW handle: {}", DumpWindow(info.Taskbar.WorkerWWindow));
		MessagePrint(spdlog::level::off, buf);

		MessagePrint(spdlog::level::off, L"\tMaximised windows:");
		DumpWindowSet(L"\t\t\t", info.MaximisedWindows);

		MessagePrint(spdlog::level::off, L"\tNormal windows:");
		DumpWindowSet(L"\t\t\t", info.NormalWindows);
	}

	buf.clear();
	std::format_to(std::back_inserter(buf), L"User is using Aero Peek: {}", m_PeekActive);
	MessagePrint(spdlog::level::off, buf);

	buf.clear();
	std::format_to(std::back_inserter(buf), L"User is using Task View: {}", m_TaskViewActive);
	MessagePrint(spdlog::level::off, buf);

	if (m_CurrentStartMonitor != nullptr)
	{
		buf.clear();
		std::format_to(std::back_inserter(buf), L"Start menu is opened: true [monitor {}]", static_cast<void *>(m_CurrentStartMonitor));
		MessagePrint(spdlog::level::off, buf);
	}
	else
	{
		MessagePrint(spdlog::level::off, L"Start menu is opened: false");
	}

	if (m_CurrentSearchMonitor != nullptr)
	{
		buf.clear();
		std::format_to(std::back_inserter(buf), L"Search is opened: true [monitor {}]", static_cast<void *>(m_CurrentSearchMonitor));
		MessagePrint(spdlog::level::off, buf);
	}
	else
	{
		MessagePrint(spdlog::level::off, L"Search is opened: false");
	}

	buf.clear();
	std::format_to(std::back_inserter(buf), L"Current foreground window: {}", DumpWindow(m_ForegroundWindow));
	MessagePrint(spdlog::level::off, buf);

	buf.clear();
	std::format_to(std::back_inserter(buf), L"Worker handles attribute refresh requests from hooks: {}", !m_disableAttributeRefreshReply);
	MessagePrint(spdlog::level::off, buf);

	buf.clear();
	std::format_to(std::back_inserter(buf), L"Battery saver is active: {}", m_PowerSaver);
	MessagePrint(spdlog::level::off, buf);

	MessagePrint(spdlog::level::off, L"Taskbars currently using normal appearance:");
	DumpWindowSet(L"\t\t", m_NormalTaskbars, false);

	MessagePrint(spdlog::level::off, L"===== End TaskbarAttributeWorker state dump =====");
}

void TaskbarAttributeWorker::ResetState(bool manual)
{
	if (!m_ResettingState)
	{
		MessagePrint(spdlog::level::debug, L"Resetting worker state");

		m_ResettingState = true;
		m_ResetStateReentered = false;
		auto guard = wil::scope_exit([this]
		{
			m_ResettingState = false;
			m_ResetStateReentered = false;
		});

		// Clear state
		m_PowerSaver = false;
		m_PeekActive = false;
		m_TaskViewActive = false;
		m_CurrentStartMonitor = nullptr;
		m_CurrentSearchMonitor = nullptr;
		m_ForegroundWindow = Window::NullWindow;

		m_Taskbars.clear();
		m_NormalTaskbars.clear();

		// Keep old hooks alive while we rehook to avoid DLL unload.
		auto oldHooks = std::move(m_Hooks);
		m_Hooks.clear();

		if (const Window main_taskbar = Window::Find(TASKBAR))
		{
			const auto pid = main_taskbar.process_id();
			if (!manual)
			{
				if (pid != m_LastExplorerPid)
				{
					const auto now = std::chrono::steady_clock::now();
					if (now < m_LastExplorerRestart + std::chrono::seconds(30)) [[unlikely]]
					{
						MessagePrint(spdlog::level::critical, L"Windows Explorer restarted twice in the last 30 seconds! This may be a conflict between TranslucentTB and other shell customization software, or a Windows Update. To avoid further issues, TranslucentTB will now exit.");
					}

					m_LastExplorerRestart = now;
				}
			}

			m_LastExplorerPid = pid;

			InsertTaskbar(main_taskbar.monitor(), main_taskbar);
		}

		for (const Window secondtaskbar : Window::FindEnum(SECONDARY_TASKBAR))
		{
			InsertTaskbar(secondtaskbar.monitor(), secondtaskbar);
		}

		// drop the old hooks.
		oldHooks.clear();

		CreateSearchManager();

		// This might race but it's not an issue because
		// it'll race on a single thread and only ends up
		// doing something twice.

		SYSTEM_POWER_STATUS powerStatus;
		if (GetSystemPowerStatus(&powerStatus))
		{
			m_PowerSaver = powerStatus.SystemStatusFlag;
		}
		else
		{
			LastErrorHandle(spdlog::level::warn, L"Failed to verify system power status");
		}

		// TODO: check if aero peek is active

		if (const auto hookWnd = Window::Find(TTBHOOK_TASKVIEWMONITOR, TTBHOOK_TASKVIEWMONITOR); hookWnd && m_IsTaskViewOpenedMessage)
		{
			m_TaskViewActive = hookWnd.send_message(*m_IsTaskViewOpenedMessage);
		}

		m_ForegroundWindow = Window::ForegroundWindow();
		if (IsStartMenuOpened())
		{
			m_CurrentStartMonitor = GetStartMenuMonitor();
		}

		if (IsSearchOpened())
		{
			m_CurrentStartMonitor = GetSearchMonitor();
		}

		for (const Window window : Window::FindEnum())
		{
			InsertWindow(window, false);
		}

		if (!m_ResetStateReentered)
		{
			// Apply the calculated effects
			RefreshAllAttributes();
		}
		else
		{
			// we got re-entrancy, all this might not even be valid anymore, try it again.
			guard.reset();
			ResetState(manual);
		}
	}
	else
	{
		MessagePrint(spdlog::level::debug, L"ResetState re-entrancy detected");

		m_ResetStateReentered = true;
	}
}

TaskbarAttributeWorker::~TaskbarAttributeWorker() noexcept(false)
{
	m_disableAttributeRefreshReply = true;
	UnregisterSearchCallbacks();
	ReturnToStock();
}
