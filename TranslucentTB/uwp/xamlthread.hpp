#pragma once
#include "arch.h"
#include <atomic>
#include <cassert>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>
#include <windef.h>
#include <WinBase.h>
#include <wil/resource.h>
#include <windows.ui.xaml.hosting.desktopwindowxamlsource.h>
#include "winrt.hpp"
#include <winrt/Windows.System.h>
#include "undefgetcurrenttime.h"
#include <winrt/Windows.UI.Xaml.Hosting.h>
#include "redefgetcurrenttime.h"

#include "xamlpagehost.hpp"
#include "../../ProgramLog/error/winrt.hpp"
#include "util/thread_independent_mutex.hpp"
#include "util/type_traits.hpp"

class XamlThread {
private:
	wil::unique_handle m_Thread;
	wil::slim_event_manual_reset m_Ready;
	winrt::Windows::System::DispatcherQueueController m_Dispatcher;
	winrt::Windows::UI::Xaml::Hosting::WindowsXamlManager m_Manager;

	std::vector<winrt::weak_ref<IDesktopWindowXamlSourceNative2>> m_Sources;

	Util::thread_independent_mutex m_CurrentWindowLock;
	std::unique_ptr<BaseXamlPageHost> m_CurrentWindow;

	static DWORD WINAPI ThreadProc(LPVOID param);
	static void DeletedCallback(void *data);

	void ThreadInit();
	bool PreTranslateMessage(const MSG &msg);

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

	template<typename T, typename Callback, typename... Args>
	void CreateXamlWindow(std::unique_lock<Util::thread_independent_mutex> lock, HINSTANCE hInst, xaml_startup_position pos, Callback &&callback, Args&&... args)
	{
		// more than one window per thread is technically doable but impractical,
		// because XAML Islands has a ton of bugs when hosting more than 1 window per thread
		assert(m_CurrentWindow == nullptr);

		m_Dispatcher.DispatcherQueue().TryEnqueue([this, hInst, pos, lock = std::move(lock), callback = std::forward<Callback>(callback), ...args = std::forward<Args>(args)]() mutable
		{
			std::unique_ptr<XamlPageHost<T>> host;
			try
			{
				host = std::make_unique<XamlPageHost<T>>(hInst, pos, m_Dispatcher.DispatcherQueue(), DeletedCallback, this, std::forward<Util::decay_array_t<Args>>(args)...);
			}
			HresultErrorCatch(spdlog::level::critical, L"Failed to create XAML window");

			m_Sources.push_back(host->source().try_as<IDesktopWindowXamlSourceNative2>());

			std::invoke(std::forward<Callback>(callback), host->content());

			m_CurrentWindow = std::move(host);
		});
	}

	wil::unique_handle Delete();
};
