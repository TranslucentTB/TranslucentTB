#pragma once
#include "arch.h"
#include <cstdint>
#include <guiddef.h>

#include "util/null_terminated_string_view.hpp"

#pragma region App

// Mutex name for app uniqueness
static constexpr Util::null_terminated_wstring_view MUTEX_GUID = L"344635E9-9AE4-4E60-B128-D53E25AB70A7";

// Tray icon GUID
static constexpr GUID TRAY_GUID = { 0x974A6DD7, 0x6BBD, 0x4B9A, { 0x9F, 0x11, 0xA8, 0xED, 0x62, 0x44, 0x2A, 0x55 } };

#pragma endregion

#pragma region Messages

// Message sent by explorer when the taskbar is created
static constexpr Util::null_terminated_wstring_view WM_TASKBARCREATED = L"TaskbarCreated";

// Sent by the hook to the worker when the taskbar is trying to change its composition attribute
static constexpr Util::null_terminated_wstring_view WM_TTBHOOKREQUESTREFRESH = L"TTBHook_RequestAttributeRefresh";

// Sent by the hook to the worker when Task View closes/opens
static constexpr Util::null_terminated_wstring_view WM_TTBHOOKTASKVIEWVISIBILITYCHANGE = L"TTBHook_TaskViewVisibilityChange";

// Sent by the worker to the hook to get the current Task View status
static constexpr Util::null_terminated_wstring_view WM_TTBHOOKISTASKVIEWOPENED = L"TTBHook_IsTaskViewOpened";

// Sent by LauncherVisibilitySink when the start menu opens/closes
static constexpr Util::null_terminated_wstring_view WM_TTBSTARTVISIBILITYCHANGE = L"TTB_StartVisibilityChange";

// Sent by TaskbarAttributeWorker to itself to switch back to main thread
static constexpr Util::null_terminated_wstring_view WM_TTBSEARCHVISIBILITYCHANGE = L"TTB_SearchVisibilityChange";

// Sent to the worker to force the taskbar to toggle to normal and back to the expected appearance
static constexpr Util::null_terminated_wstring_view WM_TTBFORCEREFRESHTASKBAR = L"TTB_ForceRefreshTaskbar";

// Sent by another instance of TranslucentTB to signal that it was started while this instance is running.
static constexpr Util::null_terminated_wstring_view WM_TTBNEWINSTANCESTARTED = L"TTB_NewInstancecStarted";

#pragma endregion

#pragma region Window classes

// Window class used by our tray icon
static constexpr Util::null_terminated_wstring_view TRAY_WINDOW = L"TrayWindow";

// Window class and title used by our attribute worker
static constexpr Util::null_terminated_wstring_view TTB_WORKERWINDOW = L"TTB_WorkerWindow";

// Window class and title used by the Task View monitor
static constexpr Util::null_terminated_wstring_view TTBHOOK_TASKVIEWMONITOR = L"TTBHook_TaskViewMonitor";

// Window class for taskbar on primary monitor
static constexpr Util::null_terminated_wstring_view TASKBAR = L"Shell_TrayWnd";

// Window class for taskbars on other monitors
static constexpr Util::null_terminated_wstring_view SECONDARY_TASKBAR = L"Shell_SecondaryTrayWnd";

// Window class used by UWP
static constexpr Util::null_terminated_wstring_view CORE_WINDOW = L"Windows.UI.Core.CoreWindow";

#pragma endregion

#pragma region Other

// UTF-8 Byte Order Mark
static constexpr Util::null_terminated_string_view UTF8_BOM = "\xEF\xBB\xBF";

// Serialization Keys
static constexpr std::wstring_view CLASS_KEY = L"window_class";
static constexpr std::wstring_view TITLE_KEY = L"window_title";
static constexpr std::wstring_view FILE_KEY = L"process_name";

#pragma endregion
