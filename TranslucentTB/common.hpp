#pragma once
#include <cstdint>

// https://msdn.microsoft.com/en-us/library/windows/desktop/aa365247(v=vs.85).aspx
// "Note: The maximum path of 32,767 characters is approximate."
// smh
constexpr uint16_t LONG_PATH = 33000;

// Minimum build number that supports Fluent
constexpr uint16_t MIN_FLUENT_BUILD = 17063;