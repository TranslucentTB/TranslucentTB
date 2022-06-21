#pragma once
#include "arch.h"
#include <cassert>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>
#include <windef.h>
#include <WinBase.h>
#include <wil/resource.h>
#include <windows.ui.xaml.hosting.desktopwindowxamlsource.h>
#include "winrt.hpp"
#include <winrt/Windows.System.h>
#include "undefgetcurrenttime.h"
#include <winrt/Windows.UI.Xaml.Hosting.h>
#include "redefgetcurrenttime.h"
#include <wil/cppwinrt_helpers.h>

#include "xamlpagehost.hpp"
#include "../../ProgramLog/error/winrt.hpp"
#include "undoc/uxtheme.hpp"
#include "util/thread_independent_mutex.hpp"
#include "util/type_traits.hpp"

class XamlThread {
private:
	wil::unique_handle m_Thread;
	wil::slim_event_manual_reset m_Ready;
	winrt::Windows::System::DispatcherQueueController m_Dispatcher;
	wuxh::WindowsXamlManager m_Manager;

	winrt::com_ptr<IDesktopWindowXamlSourceNative2> m_Source;

	Util::thread_independent_mutex m_CurrentWindowLock;
	std::unique_ptr<BaseXamlPageHost> m_CurrentWindow;

	static DWORD WINAPI ThreadProc(LPVOID param);
	static void DeletedCallback(void *data);

	void ThreadInit();
	bool PreTranslateMessage(const MSG &msg);
	winrt::fire_and_forget ThreadDeinit();

public:
	XamlThread();

	XamlThread(const XamlThread &thread) = delete;
	XamlThread &operator =(const XamlThread &thread) = delete;

	std::unique_lock<Util::thread_independent_mutex> Lock() noexcept
	{
		return std::unique_lock { m_CurrentWindowLock };
	}

	bool IsAvailable() const noexcept
	{
		return m_CurrentWindow == nullptr;
	}

	const std::unique_ptr<BaseXamlPageHost> &GetCurrentWindow() const
	{
		return m_CurrentWindow;
	}

	template<typename T, typename Callback, typename... Args>
	winrt::fire_and_forget CreateXamlWindow(std::unique_lock<Util::thread_independent_mutex> lock, WindowClass &classRef, WindowClass& dragRegionClass, xaml_startup_position pos, Callback callback, Args... args)
	{
		// more than one window per thread is technically doable but impractical,
		// because XAML Islands has a ton of bugs when hosting more than 1 window per thread
		assert(m_CurrentWindow == nullptr);

		co_await wil::resume_foreground(GetDispatcher());

		std::unique_ptr<XamlPageHost<T>> host;
		try
		{
			host = std::make_unique<XamlPageHost<T>>(classRef, dragRegionClass, pos, m_Dispatcher.DispatcherQueue(), DeletedCallback, this, args...);
		}
		HresultErrorCatch(spdlog::level::critical, L"Failed to create XAML window");

		m_Source = host->source().try_as<IDesktopWindowXamlSourceNative2>();

		std::invoke(callback, host->content(), static_cast<BaseXamlPageHost *>(host.get()));

		m_CurrentWindow = std::move(host);
	}

	wil::unique_handle Delete();

	winrt::Windows::System::DispatcherQueue GetDispatcher() const
	{
		return m_Dispatcher.DispatcherQueue();
	}

	~XamlThread()
	{
		if (m_Manager)
		{
			m_Manager.Close();
			m_Manager = nullptr;
		}

		// let the XAML framework cleanup
		MSG msg{};
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
};
