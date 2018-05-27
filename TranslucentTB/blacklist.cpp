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
EventHook Blacklist::m_TitleHook(EVENT_OBJECT_NAMECHANGE, EVENT_OBJECT_NAMECHANGE, Blacklist::HandleNameChange, WINEVENT_OUTOFCONTEXT);

void Blacklist::Parse(const std::wstring &file)
{
	// Clear our vectors
	m_ClassBlacklist.clear();
	m_TitleBlacklist.clear();
	m_FileBlacklist.clear();

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

		if (Util::StringBeginsWith(line_lowercase, L"class"))
		{
			AddToVector(std::move(line), m_ClassBlacklist, delimiter);
		}
		else if (Util::StringBeginsWith(line_lowercase, L"title") || Util::StringBeginsWith(line_lowercase, L"windowtitle"))
		{
			AddToVector(std::move(line), m_TitleBlacklist, delimiter);
		}
		else if (Util::StringBeginsWith(line_lowercase, L"exename"))
		{
			AddToVector(std::move(line_lowercase), m_FileBlacklist, delimiter);
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
	if (m_Cache.count(window) != 0)
	{
		return m_Cache.at(window);
	}
	else
	{
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
			std::wstring exeName = Util::ToLower(window.filename());
			for (const std::wstring &value : m_FileBlacklist)
			{
				if (exeName == value)
				{
					return OutputMatchToLog(window, m_Cache[window] = true);
				}
			}
		}

		// Do it last because titles can change, so it's less reliable.
		if (m_TitleBlacklist.size() > 0)
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
	m_Cache.clear();

	if (Config::VERBOSE)
	{
		Log::OutputMessage(L"Blacklist cache cleared.");
	}
}

void Blacklist::HandleNameChange(HWINEVENTHOOK, DWORD, const HWND window, const LONG idObject, LONG, DWORD, DWORD)
{
	if (idObject == OBJID_WINDOW)
	{
		const Window eventWindow(window);

		if (m_Cache.count(eventWindow) != 0)
		{
			m_Cache.erase(eventWindow);
		}

		if (Config::VERBOSE)
		{
			Log::OutputMessage(L"Title of a window changed to \"" + eventWindow.title() + L'"');
		}
	}
}

void Blacklist::AddToVector(std::wstring line, std::vector<std::wstring> &vector, const wchar_t &delimiter)
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
		message << window.handle() << L" [" << window.classname() << L"] [" << window.filename() << L"] [" << window.title() << L']';

		Log::OutputMessage(message.str());
	}

	return isMatch;
}