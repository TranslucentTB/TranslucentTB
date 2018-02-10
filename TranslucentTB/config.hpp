#pragma once
#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <cstdint>
#include <windef.h>

namespace Config {
	LPCWSTR CONFIG_FILE = L"config.cfg";
	LPCWSTR EXCLUDE_FILE = L"dynamic-ws-exclude.csv";
	static const uint16_t CACHE_HIT_MAX = 500;
}

#endif // !CONFIG_HPP
