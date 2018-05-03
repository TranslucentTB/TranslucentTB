#pragma once
#include <cstdint>

// https://msdn.microsoft.com/en-us/library/windows/desktop/aa365247(v=vs.85).aspx
// "Note: The maximum path of 32,767 characters is approximate."
// smh
constexpr uint16_t LONG_PATH = 33000;

// Minimum build number that supports Fluent
constexpr uint16_t MIN_FLUENT_BUILD = 17063;

// Message id for app uniqueness
constexpr wchar_t ID[] = L"344635E9-9AE4-4E60-B128-D53E25AB70A7";

// App name
constexpr wchar_t NAME[] = L"TranslucentTB";

// Config file name
constexpr wchar_t CONFIG_FILE[] = L"config.cfg";

// Dynamic windows exclude file name
constexpr wchar_t EXCLUDE_FILE[] = L"dynamic-ws-exclude.csv";

// Message sent by explorer when the taskbar is created
constexpr wchar_t WM_TASKBARCREATED[] = L"TaskbarCreated";

// Message used by a new instance to close the old instance
constexpr wchar_t NEW_TTB_INSTANCE[] = L"NewTTBInstance";
