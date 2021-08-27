#include "taskviewvisibilitymonitor.hpp"
#include <combaseapi.h>
#include <processthreadsapi.h>
#include <synchapi.h>
#include <wil/result.h>
#include <wil/win32_helpers.h>
#include <WinUser.h>

#include "appinfo.hpp"
#include "constants.hpp"
#include "multitaskingviewvisibilitysink.hpp"
#include "undoc/explorer.hpp"

std::atomic<bool> TaskViewVisibilityMonitor::s_ThreadRunning = false;
unique_handle_failfast TaskViewVisibilityMonitor::s_ThreadCleanupEvent;
UINT TaskViewVisibilityMonitor::s_TaskViewVisibilityChangeMessage;
UINT TaskViewVisibilityMonitor::s_IsTaskViewOpenedMessage;
TaskViewVisibilityMonitor::unique_class_atom_failfast TaskViewVisibilityMonitor::s_WindowClassAtom;
unique_handle_failfast TaskViewVisibilityMonitor::s_hThread;
wil::com_ptr_failfast<IMultitaskingViewVisibilityService> TaskViewVisibilityMonitor::s_ViewService;

void TaskViewVisibilityMonitor::UnregisterClassFailFast(ATOM atom) noexcept
{
	FAIL_FAST_IF_WIN32_BOOL_FALSE(UnregisterClass(reinterpret_cast<LPCWSTR>(atom), wil::GetModuleInstanceHandle()));
}

void TaskViewVisibilityMonitor::DestroyWindowFailFast(HWND hwnd) noexcept
{
	FAIL_FAST_IF_WIN32_BOOL_FALSE(DestroyWindow(hwnd));
}

void TaskViewVisibilityMonitor::UnregisterSink(IMultitaskingViewVisibilityService *source, DWORD cookie) noexcept
{
	FAIL_FAST_IF_FAILED(source->Unregister(cookie));
}

void TaskViewVisibilityMonitor::ResetViewService() noexcept
{
	s_ViewService.reset();
}

TaskViewVisibilityMonitor::unique_view_service TaskViewVisibilityMonitor::LoadViewService() noexcept
{
	// if we get injected at process creation, it's possible the immersive shell isn't ready yet.
	// try until it is ready. if after 10 seconds the class is still not registered, it's most likely
	// an issue other than process init not being done.
	wil::com_ptr_failfast<IServiceProvider> servProv;
	uint8_t attempts = 0;
	HRESULT hr = S_OK;
	do
	{
		hr = CoCreateInstance(CLSID_ImmersiveShell, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(servProv.put()));
		if (SUCCEEDED(hr))
		{
			break;
		}
		else
		{
			++attempts;
			Sleep(500);
		}
	}
	while (hr == REGDB_E_CLASSNOTREG && attempts < 20);
	FAIL_FAST_IF_FAILED(hr);

	// on Windows 11, we frequently get not implemented if this is done too early, so apply the same treatment for the QueryService call.
	attempts = 0;
	hr = S_OK;
	do
	{
		hr = servProv->QueryService(SID_MultitaskingViewVisibilityService, s_ViewService.put());
		if (SUCCEEDED(hr))
		{
			return unique_view_service();
		}
		else
		{
			++attempts;
			Sleep(500);
		}
	}
	while (hr == E_NOTIMPL && attempts < 20);
	FAIL_FAST_HR(hr);
}

TaskViewVisibilityMonitor::unique_multitasking_view_visibility_token TaskViewVisibilityMonitor::RegisterSink() noexcept
{
	const auto sink = Microsoft::WRL::Make<MultitaskingViewVisibilitySink>(s_TaskViewVisibilityChangeMessage);
	FAIL_FAST_IF_NULL_ALLOC(sink);

	DWORD cookie = 0;
	FAIL_FAST_IF_FAILED(s_ViewService->Register(sink.Get(), &cookie));

	return { s_ViewService.get(), cookie };
}

void TaskViewVisibilityMonitor::ThreadMain() noexcept
{
	s_ThreadRunning = true;
	const auto guard = wil::scope_exit([]() noexcept
	{
		s_ThreadRunning = false;
		FAIL_FAST_IF_WIN32_BOOL_FALSE(SetEvent(s_ThreadCleanupEvent.get()));
	});

	const auto coInitialize = wil::CoInitializeEx_failfast(COINIT_APARTMENTTHREADED);
	const auto viewService = LoadViewService();
	const auto token = RegisterSink();

	unique_window_failfast window(CreateWindowEx(WS_EX_NOREDIRECTIONBITMAP, reinterpret_cast<LPCWSTR>(s_WindowClassAtom.get()), TTBHOOK_TASKVIEWMONITOR.c_str(), 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, wil::GetModuleInstanceHandle(), nullptr));
	FAIL_FAST_LAST_ERROR_IF_NULL(window);

	BOOL ret;
	MSG msg;
	while ((ret = GetMessage(&msg, nullptr, 0, 0)) != 0)
	{
		FAIL_FAST_LAST_ERROR_IF(ret == -1);

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

DWORD WINAPI TaskViewVisibilityMonitor::ThreadProc(LPVOID) noexcept
{
	ThreadMain();

	// suspend this thread forever.
	// if we just return, the code in Uninstall might terminate the thread while the CRT is
	// cleaning up thread-local structures in a lock, resulting in an abandonned lock.
	// so once the CRT is unloaded, it deadlocks the GUI thread.
	Sleep(INFINITE);

	// if the thread somehow gets resumed, something is very wrong.
	__fastfail(FAST_FAIL_FATAL_APP_EXIT);

	return static_cast<DWORD>(-1); // make the compiler happy
}

LRESULT CALLBACK TaskViewVisibilityMonitor::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept
{
	if (uMsg == s_IsTaskViewOpenedMessage)
	{
		MULTITASKING_VIEW_TYPES flags = MVT_NONE;
		const HRESULT hr = s_ViewService->IsViewVisible(MVT_ALL_UP_VIEW, &flags);
		if (SUCCEEDED(hr))
		{
			return flags & MVT_ALL_UP_VIEW;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
}

void TaskViewVisibilityMonitor::EndWatcherThread() noexcept
{
	// check if the thread is still alive
	const DWORD waitResult = WaitForSingleObject(s_hThread.get(), 0);
	if (waitResult == WAIT_TIMEOUT)
	{
		// if the DLL gets unloaded before the thread began
		// executing, we still need to terminate it.
		if (s_ThreadRunning)
		{
			const DWORD tid = GetThreadId(s_hThread.get());
			FAIL_FAST_LAST_ERROR_IF(!tid);

			FAIL_FAST_IF_WIN32_BOOL_FALSE(PostThreadMessage(tid, WM_QUIT, 0, 0));

			const DWORD shutdownWaitResult = WaitForSingleObject(s_ThreadCleanupEvent.get(), INFINITE);
			FAIL_FAST_LAST_ERROR_IF(shutdownWaitResult != WAIT_OBJECT_0);
		}

		// terminate it
		FAIL_FAST_IF_WIN32_BOOL_FALSE(TerminateThread(s_hThread.get(), 0));
	}
	else
	{
		FAIL_FAST_LAST_ERROR_IF(waitResult != WAIT_OBJECT_0);
	}
}

void TaskViewVisibilityMonitor::Install() noexcept
{
	if (!s_TaskViewVisibilityChangeMessage)
	{
		s_TaskViewVisibilityChangeMessage = RegisterWindowMessage(WM_TTBHOOKTASKVIEWVISIBILITYCHANGE.c_str());
		FAIL_FAST_LAST_ERROR_IF(!s_TaskViewVisibilityChangeMessage);
	}

	if (!s_IsTaskViewOpenedMessage)
	{
		s_IsTaskViewOpenedMessage = RegisterWindowMessage(WM_TTBHOOKISTASKVIEWOPENED.c_str());
		FAIL_FAST_LAST_ERROR_IF(!s_IsTaskViewOpenedMessage);
	}

	if (!s_WindowClassAtom)
	{
		const WNDCLASSEX wndClass = {
			.cbSize = sizeof(wndClass),
			.lpfnWndProc = WindowProc,
			.hInstance = wil::GetModuleInstanceHandle(),
			.lpszClassName = TTBHOOK_TASKVIEWMONITOR.c_str()
		};

		s_WindowClassAtom.reset(RegisterClassEx(&wndClass));
		FAIL_FAST_LAST_ERROR_IF_NULL(s_WindowClassAtom);
	}

	if (!s_ThreadCleanupEvent)
	{
		s_ThreadCleanupEvent.reset(CreateEvent(nullptr, true, false, nullptr));
		FAIL_FAST_LAST_ERROR_IF_NULL(s_WindowClassAtom);
	}

	if (!s_hThread)
	{
		s_hThread.reset(CreateThread(nullptr, 0, ThreadProc, wil::GetModuleInstanceHandle(), 0, nullptr));
		FAIL_FAST_LAST_ERROR_IF_NULL(s_hThread);

#ifdef _DEBUG
		FAIL_FAST_IF_FAILED(SetThreadDescription(s_hThread.get(), APP_NAME L" Task View Visibility Monitor Thread"));
#endif
	}
}

void TaskViewVisibilityMonitor::Uninstall() noexcept
{
	if (s_hThread)
	{
		EndWatcherThread();
		s_hThread.reset();
	}
}
