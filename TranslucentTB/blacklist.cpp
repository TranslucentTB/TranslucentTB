#include "blacklist.hpp"
#include <fstream>
#include <sstream>

#include "config.hpp"
#include "ttblog.hpp"
#include "util/strings.hpp"

std::unordered_set<std::wstring> Blacklist::s_ClassBlacklist;
std::unordered_set<std::wstring> Blacklist::s_FileBlacklist;
std::vector<std::wstring> Blacklist::s_TitleBlacklist;

std::unordered_map<Window, bool> Blacklist::s_Cache;

const EventHook Blacklist::s_ChangeHook(EVENT_OBJECT_NAMECHANGE, Blacklist::HandleChangeEvent);
const EventHook Blacklist::s_DestroyHook(EVENT_OBJECT_DESTROY, Blacklist::HandleDestroyEvent);

void Blacklist::Parse(const std::filesystem::path &file)
{
	// Clear our vectors
	s_ClassBlacklist.clear();
	s_TitleBlacklist.clear();
	s_FileBlacklist.clear();

	const wchar_t delimiter = L',';
	const wchar_t comment = L';';

	std::wifstream excludesfilestream(file);
	for (std::wstring line; std::getline(excludesfilestream, line);)
	{
		Util::TrimInplace(line);
		if (line.empty())
		{
			continue;
		}

		size_t comment_index = line.find(comment);
		if (comment_index == 0)
		{
			continue;
		}
		else if (comment_index != std::wstring::npos)
		{
			line.erase(comment_index);
		}

		if (line[line.length() - 1] != delimiter)
		{
			line += delimiter;
		}

		std::wstring line_lowercase = Util::ToLower(line);

		if (line_lowercase.starts_with(L"class"))
		{
			AddToSet(line, s_ClassBlacklist, delimiter);
		}
		else if (Util::StringBeginsWithOneOf(line_lowercase, { L"title", L"windowtitle" }))
		{
			AddToVector(line, s_TitleBlacklist, delimiter);
		}
		else if (line_lowercase.starts_with(L"exename"))
		{
			AddToSet(line_lowercase, s_FileBlacklist, delimiter);
		}
		else
		{
			Log::OutputMessage(L"Invalid line in dynamic window blacklist file.");
		}
	}

	ClearCache();
}

bool Blacklist::IsBlacklisted(Window window)
{
	if (s_Cache.contains(window))
	{
		return s_Cache.at(window);
	}
	else
	{
		// This is the fastest because we do the less string manipulation, so always try it first
		if (!s_ClassBlacklist.empty())
		{
			if (s_ClassBlacklist.contains(window.classname()))
			{
				return OutputMatchToLog(window, s_Cache[window] = true);
			}
		}

		if (!s_FileBlacklist.empty())
		{
			if (s_FileBlacklist.contains(Util::ToLower(window.file().filename().native())))
			{
				return OutputMatchToLog(window, s_Cache[window] = true);
			}
		}

		// Do it last because titles can change, so it's less reliable.
		if (!s_TitleBlacklist.empty())
		{
			for (const std::wstring &value : s_TitleBlacklist)
			{
				if (window.title().find(value) != std::wstring::npos)
				{
					return OutputMatchToLog(window, s_Cache[window] = true);
				}
			}
		}

		return OutputMatchToLog(window, s_Cache[window] = false);
	}
}

void Blacklist::ClearCache()
{
	s_Cache.clear();

	if (Config::VERBOSE)
	{
		Log::OutputMessage(L"Blacklist cache cleared.");
	}
}

void Blacklist::HandleChangeEvent(DWORD, Window window, ...)
{
	s_Cache.erase(window);
}

void Blacklist::HandleDestroyEvent(DWORD, Window window, ...)
{
	s_Cache.erase(window);
}

void Blacklist::AddToContainer(std::wstring_view line, wchar_t delimiter, const std::function<void(std::wstring_view)> &inserter)
{
	size_t pos;

	// First lets remove the key
	if ((pos = line.find(delimiter)) != std::wstring::npos)
	{
		line.remove_prefix(pos + 1);
	}

	// Now iterate and add the values
	while ((pos = line.find(delimiter)) != std::wstring::npos)
	{
		inserter(Util::Trim(line.substr(0, pos)));
		line.remove_prefix(pos + 1);
	}
}

void Blacklist::AddToVector(std::wstring_view line, std::vector<std::wstring> &vector, wchar_t delimiter)
{
	AddToContainer(line, delimiter, [&vector](std::wstring_view line)
	{
		vector.emplace_back(line);
	});
}

void Blacklist::AddToSet(std::wstring_view line, std::unordered_set<std::wstring> &set, wchar_t delimiter)
{
	AddToContainer(line, delimiter, [&set](std::wstring_view line)
	{
		set.emplace(line);
	});
}

bool Blacklist::OutputMatchToLog(Window window, bool isMatch)
{
	if (Config::VERBOSE)
	{
		std::wostringstream message;
		message << (isMatch ? L"B" : L"No b") << L"lacklist match found for window: ";
		message << window.handle() << L" [" << window.classname() << L"] [" << window.file().filename() << L"] [" << window.title() << L']';

		Log::OutputMessage(message.str());
	}

	return isMatch;
}