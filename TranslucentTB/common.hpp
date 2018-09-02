#pragma once
#include <cstdint>

// https://msdn.microsoft.com/en-us/library/windows/desktop/aa365247(v=vs.85).aspx
// "Note: The maximum path of 32,767 characters is approximate."
// smh
static constexpr uint16_t LONG_PATH = 33000;

// Minimum build number that supports Fluent
static constexpr uint16_t MIN_FLUENT_BUILD = 17063;

// Mutex name for app uniqueness
static constexpr wchar_t MUTEX_GUID[] = L"344635E9-9AE4-4E60-B128-D53E25AB70A7";

// App name
// Using a define so when can concatenate it in strings just by putting another one near it
#define NAME L"TranslucentTB"

// Config file name
static constexpr wchar_t CONFIG_FILE[] = L"config.cfg";

// Dynamic windows exclude file name
static constexpr wchar_t EXCLUDE_FILE[] = L"dynamic-ws-exclude.csv";

// Message sent by explorer when the taskbar is created
static constexpr wchar_t WM_TASKBARCREATED[] = L"TaskbarCreated";

// Message used by a new instance to close the old instance
static constexpr wchar_t NEW_TTB_INSTANCE[] = L"NewTTBInstance";

// Window class used by UWP
static constexpr wchar_t CORE_WINDOW[] = L"Windows.UI.Core.CoreWindow";