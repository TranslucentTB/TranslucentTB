#include "blacklist.hpp"
#include <fstream>
#include <sstream>

#include "config.hpp"
#include "ttblog.hpp"
#include "util.hpp"

std::vector<std::wstring> Blacklist::m_ClassBlacklist;
std::vector<std::wstring> Blacklist::m_FileBlacklist;
std::vector<std::wstring> Blacklist::m_TitleBlacklist;
std::unordered_map<Window, bool> Blacklist::m_Cache;
uint16_t Blacklist::m_CacheHits;

void Blacklist::Parse(const std::wstring &file)
{
	// Clear our vectors
	m_ClassBlacklist.clear();
	m_TitleBlacklist.clear();
	m_FileBlacklist.clear();

	std::wifstream excludesfilestream(file);

	const wchar_t delimiter = L',';
	const wchar_t comment = L';';

	for (std::wstring line; std::getline(excludesfilestream, line);)
	{
		line = Util::Trim(line);
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
			line = line.substr(0, comment_index);
		}

		if (line[line.length() - 1] != delimiter)
		{
			line += delimiter;
		}

		std::wstring line_lowercase = line;
		Util::ToLower(line_lowercase);

		if (line_lowercase.substr(0, 5) == L"class")
		{
			AddToVector(delimiter, m_ClassBlacklist, line);
		}
		else if (line_lowercase.substr(0, 5) == L"title" || line.substr(0, 13) == L"windowtitle")
		{
			AddToVector(delimiter, m_TitleBlacklist, line);
		}
		else if (line_lowercase.substr(0, 7) == L"exename")
		{
			AddToVector(delimiter, m_FileBlacklist, line_lowercase);
		}
		else
		{
			Log::OutputMessage(L"Invalid line in dynamic window blacklist file");
		}
	}

	ClearCache();
}

bool Blacklist::IsBlacklisted(const Window &window)
{
	const bool has_title_blacklist = m_TitleBlacklist.size() > 0;

	if ((!has_title_blacklist || m_CacheHits <= Config::CACHE_HIT_MAX) && m_Cache.count(window) > 0)
	{
		m_CacheHits++;
		return m_Cache[window];
	}
	else
	{
		if (has_title_blacklist && m_CacheHits > Config::CACHE_HIT_MAX)
		{
			if (Config::VERBOSE)
			{
				Log::OutputMessage(L"Maximum number of cache hits reached, clearing blacklist cache.");
			}
			ClearCache();
		}

		// This is the fastest because we do the less string manipulation, so always try it first
		if (m_ClassBlacklist.size() > 0)
		{
			for (const std::wstring &value : m_ClassBlacklist)
			{
				if (window.classname() == value)
				{
					return OutputMatchToLog(window, m_Cache[window] = true);
				}
			}
		}

		if (m_FileBlacklist.size() > 0)
		{
			std::wstring exeName = window.filename();
			Util::ToLower(exeName);
			for (const std::wstring &value : m_FileBlacklist)
			{
				if (exeName == value)
				{
					return OutputMatchToLog(window, m_Cache[window] = true);
				}
			}
		}

		// Do it last because titles can change, so it's less reliable.
		if (has_title_blacklist)
		{
			const std::wstring title = window.title();
			for (const std::wstring &value : m_TitleBlacklist)
			{
				if (title.find(value) != std::wstring::npos)
				{
					return OutputMatchToLog(window, m_Cache[window] = true);
				}
			}
		}

		return OutputMatchToLog(window, m_Cache[window] = false);
	}
}

void Blacklist::ClearCache()
{
	m_CacheHits = 0;
	m_Cache.clear();

	if (Config::VERBOSE)
	{
		Log::OutputMessage(L"Blacklist cache cleared.");
	}
}

void Blacklist::AddToVector(const wchar_t &delimiter, std::vector<std::wstring> &vector, std::wstring line)
{
	size_t pos;

	// First lets remove the key
	if ((pos = line.find(delimiter)) != std::wstring::npos)
	{
		line.erase(0, pos + 1);
	}

	// Now iterate and add the values
	while ((pos = line.find(delimiter)) != std::wstring::npos)
	{
		vector.push_back(Util::Trim(line.substr(0, pos)));
		line.erase(0, pos + 1);
	}
}

bool Blacklist::OutputMatchToLog(const Window &window, const bool &isMatch)
{
	if (Config::VERBOSE)
	{
		std::wostringstream message;
		message << (isMatch ? L"B" : L"No b") << L"lacklist match found for window: ";
		message << window.handle() << L" [" << window.classname() << L"] [" << window.filename() << L"] [" << window.title() << L"]";

		Log::OutputMessage(message.str());
	}

	return isMatch;
}
