#pragma once
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "eventhook.hpp"
#include "window.hpp"

class Blacklist {
public:
	static void Parse(const std::wstring &file);
	static bool IsBlacklisted(const Window &window);
	static void ClearCache();

private:
	static std::unordered_set<std::wstring> m_ClassBlacklist;
	static std::unordered_set<std::wstring> m_FileBlacklist;
	static std::vector<std::wstring> m_TitleBlacklist;

	static std::recursive_mutex m_CacheLock;
	static std::unordered_map<Window, bool> m_Cache;

	friend class Hooks;

	static void AddToVector(std::wstring line, std::vector<std::wstring> &vector, const wchar_t &delimiter = L',');
	static void AddToSet(std::wstring line, std::unordered_set<std::wstring> &set, const wchar_t &delimiter = L',');
	static const bool &OutputMatchToLog(const Window &window, const bool &isMatch);
};