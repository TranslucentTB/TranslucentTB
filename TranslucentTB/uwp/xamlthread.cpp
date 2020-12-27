#include "xamlthread.hpp"
#include <CoreWindow.h>
#include <wil/resource.h>
#include <winrt/Windows.UI.Core.h>

#include "../windows/window.hpp"
#include "uwp.hpp"
#include "../../ProgramLog/error/win32.hpp"

DWORD WINAPI XamlThread::ThreadProc(LPVOID param)
{
	const auto that = static_cast<XamlThread *>(param);
	that->ThreadInit();
	that->m_Ready.SetEvent();

	BOOL ret;
	MSG msg;
	while ((ret = GetMessage(&msg, Window::NullWindow, 0, 0)) != 0)
	{
		if (ret != -1)
		{
			if (!that->PreTranslateMessage(msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else
		{
			LastErrorHandle(spdlog::level::critical, L"Failed to get message");
		}
	}

	delete that;
	HresultVerify(BufferedPaintUnInit(), spdlog::level::warn, L"Failed to uninitialize buffered paint");
	winrt::uninit_apartment();
	return static_cast<DWORD>(msg.wParam);
}

void XamlThread::DeletedCallback(void *data)
{
	const auto that = static_cast<XamlThread *>(data);

	std::scoped_lock guard(that->m_CurrentWindowLock);
	that->m_CurrentWindow.reset();
}

void XamlThread::ThreadInit()
{
	try
	{
		winrt::init_apartment(winrt::apartment_type::single_threaded);
	}
	HresultErrorCatch(spdlog::level::critical, L"Failed to initialize thread apartment");

	m_Dispatcher = UWP::CreateDispatcherController();

	try
	{
		m_Manager = wuxh::WindowsXamlManager::InitializeForCurrentThread();
	}
	HresultErrorCatch(spdlog::level::critical, L"Failed to create XAML manager");

	Window coreWin;
	try
	{
		const auto coreInterop = winrt::Windows::UI::Core::CoreWindow::GetForCurrentThread().as<ICoreWindowInterop>();
		winrt::check_hresult(coreInterop->get_WindowHandle(coreWin.put()));
	}
	HresultErrorCatch(spdlog::level::warn, L"Failed to get core window handle");

	if (coreWin)
	{
		if (!coreWin.show(SW_HIDE))
		{
			LastErrorHandle(spdlog::level::warn, L"Failed to hide core window");
		}
	}

	HresultVerify(BufferedPaintInit(), spdlog::level::warn, L"Failed to initialize buffered paint");
}

bool XamlThread::PreTranslateMessage(const MSG &msg)
{
	// prevent XAML islands from capturing ALT-{F4,SPACE} because of
	// https://github.com/microsoft/microsoft-ui-xaml/issues/2408
	if (msg.message == WM_SYSKEYDOWN && (msg.wParam == VK_F4 || msg.wParam == VK_SPACE)) [[unlikely]]
	{
		Window(msg.hwnd).ancestor(GA_ROOT).send_message(msg.message, msg.wParam, msg.lParam);
		return true;
	}

	for (auto it = m_Sources.begin(); it != m_Sources.end();)
	{
		if (const auto source = it->get())
		{
			BOOL result;
			const HRESULT hr = source->PreTranslateMessage(&msg, &result);
			if (SUCCEEDED(hr))
			{
				if (result)
				{
					return result;
				}
			}
			else
			{
				HresultHandle(hr, spdlog::level::warn, L"Failed to pre-translate message");
			}

			++it;
		}
		else
		{
			it = m_Sources.erase(it);
		}
	}

	return false;
}

XamlThread::XamlThread() :
	m_Dispatcher(nullptr),
	m_Manager(nullptr)
{
	m_Thread.reset(CreateThread(nullptr, 0, XamlThread::ThreadProc, this, 0, nullptr));
	if (!m_Thread)
	{
		LastErrorHandle(spdlog::level::critical, L"Failed to create XAML thread");
	}

	m_Ready.wait();
}

wil::unique_handle XamlThread::Delete()
{
	m_Dispatcher.DispatcherQueue().TryEnqueue(winrt::Windows::System::DispatcherQueuePriority::Low, [this]() -> winrt::fire_and_forget
	{
		// only called during destruction of thread pool, so no locking needed.
		m_CurrentWindow.reset();

		m_Manager.Close();
		m_Manager = nullptr;

		co_await m_Dispatcher.ShutdownQueueAsync();
		PostQuitMessage(0);
	});

	return std::move(m_Thread);
}
