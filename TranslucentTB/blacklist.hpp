#pragma once
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "smart/eventhook.hpp"
#include "util/strings.hpp"
#include "windows/window.hpp"

class Blacklist {
public:
	static void Parse(const std::filesystem::path &file);
	static bool IsBlacklisted(Window window);
	static void ClearCache();

private:
	static std::unordered_set<std::wstring> s_ClassBlacklist;
	static Util::string_set s_FileBlacklist;
	static std::vector<std::wstring> s_TitleBlacklist;

	static std::unordered_map<Window, bool> s_Cache;

	static const EventHook s_ChangeHook;
	static const EventHook s_DestroyHook;

	static void HandleChangeEvent(DWORD, Window window, ...);
	static void HandleDestroyEvent(DWORD, Window window, ...);

	inline static void AddToContainer(std::wstring_view line, wchar_t delimiter, const std::function<void(std::wstring_view)> &inserter)
	{
		size_t pos;

		// First lets remove the key
		if ((pos = line.find(delimiter)) != std::wstring_view::npos)
		{
			line.remove_prefix(pos + 1);
		}

		// Now iterate and add the values
		while ((pos = line.find(delimiter)) != std::wstring_view::npos)
		{
			inserter(Util::Trim(line.substr(0, pos)));
			line.remove_prefix(pos + 1);
		}
	}

	inline static void AddToVector(std::wstring_view line, std::vector<std::wstring> &vector, wchar_t delimiter = L',')
	{
		AddToContainer(line, delimiter, [&vector](std::wstring_view line)
		{
			vector.emplace_back(line);
		});
	}

	template<class hash, class compare>
	inline static void AddToSet(std::wstring_view line, std::unordered_set<std::wstring, hash, compare> &set, wchar_t delimiter = L',')
	{
		AddToContainer(line, delimiter, [&set](std::wstring_view line)
		{
			set.emplace(line);
		});
	}

	static bool OutputMatchToLog(Window window, bool isMatch);
};