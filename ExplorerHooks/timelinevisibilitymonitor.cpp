#include "timelinevisibilitymonitor.hpp"
#include <combaseapi.h>
#include <processthreadsapi.h>
#include <synchapi.h>
#include <WinUser.h>
#include <wil/win32_helpers.h>

#include "constants.hpp"
#include "multitaskingviewvisibilitysink.hpp"
#include "undoc/explorer.hpp"
#include "util/abort.hpp"
#include "wilx.hpp"

std::atomic<bool> TimelineVisibilityMonitor::s_ThreadRunning = false;
wil::slim_event_manual_reset TimelineVisibilityMonitor::s_ThreadCleanupDone;
wil::slim_event_manual_reset TimelineVisibilityMonitor::s_ThreadInfinitePostCleanupWait;
UINT TimelineVisibilityMonitor::s_TimelineNotification;
UINT TimelineVisibilityMonitor::s_GetTimelineStatus;
ATOM TimelineVisibilityMonitor::s_WindowClassAtom;
HANDLE TimelineVisibilityMonitor::s_hThread;
wil::com_ptr_nothrow<IMultitaskingViewVisibilityService> TimelineVisibilityMonitor::s_ViewService;

HRESULT TimelineVisibilityMonitor::LoadViewService() noexcept
{
	// if we get injected at process creation, it's possible the immersive shell isn't ready yet.
	// try until it is ready. if after 10 seconds the class is still not registered, it's most likely
	// an issue other than process init not being done.
	wil::com_ptr_nothrow<IServiceProvider> servProv;
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

	if (SUCCEEDED(hr))
	{
		hr = servProv->QueryService(SID_MultitaskingViewVisibilityService, s_ViewService.put());
	}

	return hr;
}

DWORD WINAPI TimelineVisibilityMonitor::ThreadProc(LPVOID) noexcept
{
	s_ThreadRunning = true;

	HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	if (FAILED(hr)) [[unlikely]]
	{
		Util::QuickAbort();
	}

	wil::unique_couninitialize_call coUninit;

	hr = LoadViewService();
	if (FAILED(hr)) [[unlikely]]
	{
		Util::QuickAbort();
	}

	wil::com_ptr_nothrow<MultitaskingViewVisibilitySink> sink(Microsoft::WRL::Make<MultitaskingViewVisibilitySink>());
	if (!sink) [[unlikely]]
	{
		Util::QuickAbort();
	}

	DWORD token = 0;
	hr = s_ViewService->Register(sink.get(), &token);
	if (FAILED(hr)) [[unlikely]]
	{
		Util::QuickAbort();
	}

	sink.reset(); // we don't need the sink anymore

	HWND window = CreateWindowEx(WS_EX_NOREDIRECTIONBITMAP, reinterpret_cast<LPCWSTR>(s_WindowClassAtom), HOOK_MONITOR_WINDOW.c_str(), 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, wil::GetModuleInstanceHandle(), nullptr);
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

	hr = s_ViewService->Unregister(token);
	if (FAILED(hr)) [[unlikely]]
	{
		Util::QuickAbort();
	}

	s_ViewService.reset();
	coUninit.reset();

	s_ThreadCleanupDone.SetEvent();

	// wait forever
	// if we just return, the code in Uninstall might terminate the thread while the CRT is
	// cleaning up thread-local structures in a lock, resulting in an abandonned lock.
	// so once the CRT is unloaded, it deadlocks the GUI thread.
	// this is just an event that we never signal.
	if (!s_ThreadInfinitePostCleanupWait.wait()) [[unlikely]]
	{
		Util::QuickAbort();
	}

	return static_cast<DWORD>(msg.wParam);
}

LRESULT CALLBACK TimelineVisibilityMonitor::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept
{
	if (uMsg == s_GetTimelineStatus)
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

void TimelineVisibilityMonitor::Install() noexcept
{
	if (!s_TimelineNotification)
	{
		s_TimelineNotification = RegisterWindowMessage(WM_TTBHOOKTIMELINENOTIFICATION.c_str());
		if (!s_TimelineNotification) [[unlikely]]
		{
			Util::QuickAbort();
		}
	}

	if (!s_GetTimelineStatus)
	{
		s_GetTimelineStatus = RegisterWindowMessage(WM_TTBHOOKGETTIMELINESTATUS.c_str());
		if (!s_GetTimelineStatus) [[unlikely]]
		{
			Util::QuickAbort();
		}
	}

	if (!s_WindowClassAtom)
	{
		const WNDCLASSEX wndClass = {
			.cbSize = sizeof(wndClass),
			.lpfnWndProc = WindowProc,
			.hInstance = wil::GetModuleInstanceHandle(),
			.lpszClassName = HOOK_MONITOR_WINDOW.c_str()
		};

		s_WindowClassAtom = RegisterClassEx(&wndClass);
		if (!s_WindowClassAtom) [[unlikely]]
		{
			Util::QuickAbort();
		}
	}

	if (!s_hThread)
	{
		s_hThread = CreateThread(nullptr, 0, ThreadProc, nullptr, 0, nullptr);
		if (!s_hThread) [[unlikely]]
		{
			Util::QuickAbort();
		}
	}
}

void TimelineVisibilityMonitor::Uninstall() noexcept
{
	if (s_hThread)
	{
		// check if the thread is still alive
		const DWORD waitResult = WaitForSingleObject(s_hThread, 0);
		if (waitResult == WAIT_TIMEOUT)
		{
			// if the DLL gets unloaded before the thread began
			// executing, we still need to terminate it.
			if (s_ThreadRunning)
			{
				// signal the thread and wait for its cleanup to be done
				const DWORD tid = GetThreadId(s_hThread);
				if (!tid) [[unlikely]]
				{
					Util::QuickAbort();
				}

				if (!PostThreadMessage(tid, WM_QUIT, 0, 0)) [[unlikely]]
				{
					Util::QuickAbort();
				}

				if (!s_ThreadCleanupDone.wait()) [[unlikely]]
				{
					Util::QuickAbort();
				}
			}

			// terminate it
			if (!TerminateThread(s_hThread, 0)) [[unlikely]]
			{
				Util::QuickAbort();
			}
		}
		else if (waitResult != WAIT_OBJECT_0)
		{
			// WaitForSingleObject failed
			Util::QuickAbort();
		}

		if (CloseHandle(s_hThread))
		{
			s_hThread = nullptr;
		}
		else
		{
			Util::QuickAbort();
		}
	}

	if (s_WindowClassAtom)
	{
		if (UnregisterClass(reinterpret_cast<LPCWSTR>(s_WindowClassAtom), wil::GetModuleInstanceHandle()))
		{
			s_WindowClassAtom = 0;
		}
		else
		{
			Util::QuickAbort();
		}
	}
}
