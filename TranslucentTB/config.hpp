#pragma once
#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <cstdint>
#include <cwchar>

namespace Config {

	static const std::wstring CONFIG_FILE = L"config.cfg";
	static const std::wstring EXCLUDE_FILE = L"dynamic-ws-exclude.csv";
	static uint16_t CACHE_HIT_MAX = 500;
#ifdef _DEBUG
	static bool VERBOSE = true;
#else
	static bool VERBOSE = false;
#endif

}

#endif // !CONFIG_HPP
