#include "taskviewvisibilitymonitor.hpp"
#include <combaseapi.h>
#include <processthreadsapi.h>
#include <synchapi.h>
#include <WinUser.h>

#include "constants.hpp"
#include "multitaskingviewvisibilitysink.hpp"
#include "undoc/explorer.hpp"
#include "util/abort.hpp"

using Microsoft::WRL::ComPtr;

std::atomic<bool> TaskViewVisibilityMonitor::s_ThreadRunning = false;
HANDLE TaskViewVisibilityMonitor::s_ThreadCleanupEvent;
UINT TaskViewVisibilityMonitor::s_TaskViewVisibilityChangeMessage;
UINT TaskViewVisibilityMonitor::s_IsTaskViewOpenedMessage;
ATOM TaskViewVisibilityMonitor::s_WindowClassAtom;
HANDLE TaskViewVisibilityMonitor::s_hThread;
ComPtr<IMultitaskingViewVisibilityService> TaskViewVisibilityMonitor::s_ViewService;

HRESULT TaskViewVisibilityMonitor::LoadViewService() noexcept
{
	// if we get injected at process creation, it's possible the immersive shell isn't ready yet.
	// try until it is ready. if after 10 seconds the class is still not registered, it's most likely
	// an issue other than process init not being done.
	ComPtr<IServiceProvider> servProv;
	uint8_t attempts = 0;
	HRESULT hr = S_OK;
	do
	{
		hr = CoCreateInstance(CLSID_ImmersiveShell, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(servProv.ReleaseAndGetAddressOf()));
		if (SUCCEEDED(hr))
		{
			hr = servProv->QueryService(SID_MultitaskingViewVisibilityService, s_ViewService.ReleaseAndGetAddressOf());
			break;
		}
		else
		{
			++attempts;
			Sleep(500);
		}
	}
	while (hr == REGDB_E_CLASSNOTREG && attempts < 20);

	return hr;
}

HRESULT TaskViewVisibilityMonitor::RegisterSink(DWORD &cookie) noexcept
{
	if (const auto sink = Microsoft::WRL::Make<MultitaskingViewVisibilitySink>())
	{
		return s_ViewService->Register(sink.Get(), &cookie);
	}
	else
	{
		return E_OUTOFMEMORY;
	}
}

DWORD WINAPI TaskViewVisibilityMonitor::ThreadProc(LPVOID lpParameter) noexcept
{
	s_ThreadRunning = true;

	DWORD cookie = 0;
	if (FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)) ||
		FAILED(LoadViewService()) ||
		FAILED(RegisterSink(cookie))) [[unlikely]]
	{
		Util::QuickAbort();
	}

	HWND window = CreateWindowEx(WS_EX_NOREDIRECTIONBITMAP, reinterpret_cast<LPCWSTR>(s_WindowClassAtom), TTBHOOK_TASKVIEWMONITOR.c_str(), 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, reinterpret_cast<HINSTANCE>(lpParameter), nullptr);
	if (!window) [[unlikely]]
	{
		Util::QuickAbort();
	}

	BOOL ret;
	MSG msg;
	while ((ret = GetMessage(&msg, nullptr, 0, 0)) != 0)
	{
		if (ret != -1)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			Util::QuickAbort();
		}
	}

	if (!DestroyWindow(window)) [[unlikely]]
	{
		Util::QuickAbort();
	}

	if (FAILED(s_ViewService->Unregister(cookie))) [[unlikely]]
	{
		Util::QuickAbort();
	}

	s_ViewService.Reset();
	CoUninitialize();
	s_ThreadRunning = false;

	if (!SetEvent(s_ThreadCleanupEvent))
	{
		Util::QuickAbort();
	}

	// suspend this thread
	// if we just return, the code in Uninstall might terminate the thread while the CRT is
	// cleaning up thread-local structures in a lock, resulting in an abandonned lock.
	// so once the CRT is unloaded, it deadlocks the GUI thread.
	SuspendThread(GetCurrentThread());

	// if SuspendThread fails or the thread somehow gets resumed, something is very wrong.
	Util::QuickAbort();
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

bool TaskViewVisibilityMonitor::SignalWatcherThreadAndWait(DWORD tid) noexcept
{
	// signal the thread and wait for its cleanup to be done
	return tid &&
		PostThreadMessage(tid, WM_QUIT, 0, 0) &&
		WaitForSingleObject(s_ThreadCleanupEvent, INFINITE) == WAIT_OBJECT_0;
}

bool TaskViewVisibilityMonitor::EndWatcherThread() noexcept
{
	// check if the thread is still alive
	const DWORD waitResult = WaitForSingleObject(s_hThread, 0);
	if (waitResult == WAIT_TIMEOUT)
	{
		// if the DLL gets unloaded before the thread began
		// executing, we still need to terminate it.
		if (s_ThreadRunning && !SignalWatcherThreadAndWait(GetThreadId(s_hThread)))
		{
			return false;
		}

		// terminate it
		return TerminateThread(s_hThread, 0);
	}
	else
	{
		// WaitForSingleObject failed if != WAIT_OBJECT_0
		return waitResult == WAIT_OBJECT_0;
	}
}

void TaskViewVisibilityMonitor::Install(HINSTANCE hInst) noexcept
{
	if (!s_TaskViewVisibilityChangeMessage)
	{
		s_TaskViewVisibilityChangeMessage = RegisterWindowMessage(WM_TTBHOOKTASKVIEWVISIBILITYCHANGE.c_str());
		if (!s_TaskViewVisibilityChangeMessage) [[unlikely]]
		{
			Util::QuickAbort();
		}
	}

	if (!s_IsTaskViewOpenedMessage)
	{
		s_IsTaskViewOpenedMessage = RegisterWindowMessage(WM_TTBHOOKISTASKVIEWOPENED.c_str());
		if (!s_IsTaskViewOpenedMessage) [[unlikely]]
		{
			Util::QuickAbort();
		}
	}

	if (!s_WindowClassAtom)
	{
		const WNDCLASSEX wndClass = {
			.cbSize = sizeof(wndClass),
			.lpfnWndProc = WindowProc,
			.hInstance = hInst,
			.lpszClassName = TTBHOOK_TASKVIEWMONITOR.c_str()
		};

		s_WindowClassAtom = RegisterClassEx(&wndClass);
		if (!s_WindowClassAtom) [[unlikely]]
		{
			Util::QuickAbort();
		}
	}

	if (!s_ThreadCleanupEvent)
	{
		s_ThreadCleanupEvent = CreateEvent(nullptr, true, false, nullptr);
		if (!s_ThreadCleanupEvent)
		{
			Util::QuickAbort();
		}
	}

	if (!s_hThread)
	{
		s_hThread = CreateThread(nullptr, 0, ThreadProc, hInst, 0, nullptr);
		if (!s_hThread) [[unlikely]]
		{
			Util::QuickAbort();
		}
	}
}

void TaskViewVisibilityMonitor::Uninstall(HINSTANCE hInst) noexcept
{
	if (s_hThread)
	{
		if (EndWatcherThread() && CloseHandle(s_hThread))
		{
			s_hThread = nullptr;
		}
		else
		{
			Util::QuickAbort();
		}
	}

	if (s_ThreadCleanupEvent)
	{
		if (CloseHandle(s_ThreadCleanupEvent))
		{
			s_ThreadCleanupEvent = nullptr;
		}
		else
		{
			Util::QuickAbort();
		}
	}

	if (s_WindowClassAtom)
	{
		if (UnregisterClass(reinterpret_cast<LPCWSTR>(s_WindowClassAtom), hInst))
		{
			s_WindowClassAtom = 0;
		}
		else
		{
			Util::QuickAbort();
		}
	}
}
