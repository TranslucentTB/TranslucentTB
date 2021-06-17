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

// Sent by the hook to the worker when Timeline closes/opens
static constexpr Util::null_terminated_wstring_view WM_TTBHOOKTIMELINENOTIFICATION = L"TTBHook_TimelineNotification";

// Sent by the worker to the hook to get the current Timeline status
static constexpr Util::null_terminated_wstring_view WM_TTBHOOKGETTIMELINESTATUS = L"TTBHook_GetTimelineStatus";

// Sent by LauncherVisibilitySink when the start menu opens/closes 
static constexpr Util::null_terminated_wstring_view WM_TTBSTARTVISIBILITYCHANGE = L"TTB_StartVisibilityChange";

#pragma endregion

#pragma region Window classes

// Window class used by our tray icon
static constexpr Util::null_terminated_wstring_view TRAY_WINDOW = L"TrayWindow";

// Window class and title used by our attribute worker
static constexpr Util::null_terminated_wstring_view WORKER_WINDOW = L"TTBWorkerWindow";

// Window class and title used by the Timeline monitor
static constexpr Util::null_terminated_wstring_view HOOK_MONITOR_WINDOW = L"TTBHook_TimelineMonitor";

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

#pragma endregion
