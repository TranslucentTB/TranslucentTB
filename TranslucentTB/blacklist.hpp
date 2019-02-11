#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "smart/eventhook.hpp"
#include "windows/window.hpp"

class Blacklist {
public:
	static void Parse(const std::wstring &file);
	static bool IsBlacklisted(Window window);
	static void ClearCache();

private:
	static std::unordered_set<std::wstring> m_ClassBlacklist;
	static std::unordered_set<std::wstring> m_FileBlacklist;
	static std::vector<std::wstring> m_TitleBlacklist;

	static std::unordered_map<Window, bool> m_Cache;

	static const EventHook m_ChangeHook;
	static const EventHook m_DestroyHook;

	static void HandleChangeEvent(DWORD, Window window, ...);
	static void HandleDestroyEvent(DWORD, Window window, ...);

	static void AddToContainer(std::wstring_view line, wchar_t delimiter, const std::function<void(std::wstring_view)> &inserter);
	static void AddToVector(std::wstring_view line, std::vector<std::wstring> &vector, wchar_t delimiter = L',');
	static void AddToSet(std::wstring_view line, std::unordered_set<std::wstring> &set, wchar_t delimiter = L',');
	static bool OutputMatchToLog(Window window, bool isMatch);
};