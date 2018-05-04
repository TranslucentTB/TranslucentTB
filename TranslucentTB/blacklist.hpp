#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "window.hpp"

class Blacklist {

public:
	static void Parse(const std::wstring &file);
	static bool IsBlacklisted(const Window &window);
	static void ClearCache();

private:
	static std::vector<std::wstring> m_ClassBlacklist;
	static std::vector<std::wstring> m_FileBlacklist;
	static std::vector<std::wstring> m_TitleBlacklist;

	static std::unordered_map<Window, bool> m_Cache;
	static uint16_t m_CacheHits;

	static void AddToVector(const wchar_t &delimiter, std::vector<std::wstring> &vector, std::wstring line);
	static bool OutputMatchToLog(const Window &window, const bool &isMatch);

};