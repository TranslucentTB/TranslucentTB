#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "eventhook.hpp"
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

	static EventHook m_Hook;
	static void CALLBACK HandleWinEvent(HWINEVENTHOOK, const DWORD event, const HWND window, LONG, LONG, DWORD, DWORD);

	static void AddToVector(std::wstring line, std::vector<std::wstring> &vector, const wchar_t &delimiter = L',');
	static bool OutputMatchToLog(const Window &window, const bool &isMatch);

};