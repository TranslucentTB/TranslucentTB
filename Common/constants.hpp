#pragma once
#include "arch.h"
#include <cstdint>
#include <guiddef.h>
#include <string_view>

#pragma region App

// Config file name
static constexpr std::wstring_view CONFIG_FILE = L"config.json";

// Mutex name for app uniqueness
static constexpr wchar_t MUTEX_GUID[] = L"344635E9-9AE4-4E60-B128-D53E25AB70A7";

// Tray icon GUID
static constexpr GUID TRAY_GUID = { 0x974A6DD7, 0x6BBD, 0x4B9A, { 0x9F, 0x11, 0xA8, 0xED, 0x62, 0x44, 0x2A, 0x55 } };

#pragma endregion

#pragma region Messages

// Message sent by explorer when the taskbar is created
static constexpr wchar_t WM_TASKBARCREATED[] = L"TaskbarCreated";

// Message sent to the main thread when configuration is changed
static constexpr wchar_t WM_FILECHANGED[] = L"TTB_FileChanged";

// Send by the hook to the worker when the taskbar is trying to change its composition attribute
static constexpr wchar_t WM_TTBHOOKREQUESTREFRESH[] = L"TTBHook_RequestAttributeRefresh";

#pragma endregion

#pragma region Window classes

// Window class used by our tray icon
static constexpr wchar_t TRAY_WINDOW[] = L"TrayWindow";

// Window class and title used by our attribute worker
static constexpr wchar_t WORKER_WINDOW[] = L"TTBWorkerWindow";

// Window class for taskbar on primary monitor
static constexpr wchar_t TASKBAR[] = L"Shell_TrayWnd";

// Window class for taskbars on other monitors
static constexpr wchar_t SECONDARY_TASKBAR[] = L"Shell_SecondaryTrayWnd";

// Window class used by UWP
static constexpr wchar_t CORE_WINDOW[] = L"Windows.UI.Core.CoreWindow";

#pragma endregion

#pragma region Other

// UTF-8 Byte Order Mark
static constexpr std::string_view UTF8_BOM = "\xEF\xBB\xBF";

#pragma endregion
