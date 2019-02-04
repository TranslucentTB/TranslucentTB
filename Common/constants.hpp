#pragma once
#include <cstdint>

#pragma region Windows

// https://msdn.microsoft.com/en-us/library/windows/desktop/aa365247(v=vs.85).aspx
// "Note: The maximum path of 32,767 characters is approximate."
// smh
static constexpr uint16_t LONG_PATH = 33000;

// Minimum build number that supports white system theme
static constexpr uint16_t MIN_LIGHT_BUILD = 18282;

#pragma endregion

#pragma region App

// App name
// Using a define so when can concatenate it in strings just by putting another one near it
#define NAME L"TranslucentTB"

// Config file name
static constexpr wchar_t CONFIG_FILE[] = L"config.cfg";

// Dynamic windows exclude file name
static constexpr wchar_t EXCLUDE_FILE[] = L"dynamic-ws-exclude.csv";

// Mutex name for app uniqueness
static constexpr wchar_t MUTEX_GUID[] = L"344635E9-9AE4-4E60-B128-D53E25AB70A7";

#pragma endregion

#pragma region Messages

// Message sent by explorer when the taskbar is created
static constexpr wchar_t WM_TASKBARCREATED[] = L"TaskbarCreated";

// Message used by a new instance to close the old instance
static constexpr wchar_t WM_NEWTTBINSTANCE[] = L"NewTTBInstance";

// Message sent by FolderWatcher when a file is changed.
static constexpr wchar_t WM_FILECHANGED[] = L"TTBFileChanged";

// Event hook event when Aero Peek begins
static constexpr uint32_t EVENT_SYSTEM_PEEKSTART = 0x21;

// Event hook event when Aero Peek ends
static constexpr uint32_t EVENT_SYSTEM_PEEKEND = 0x22;

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