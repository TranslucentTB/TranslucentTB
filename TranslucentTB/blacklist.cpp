#include "blacklist.hpp"

void Blacklist::Deserialize(const rapidjson::GenericValue<rapidjson::UTF16LE<>> &val)
{
	if (!val.IsObject())
	{
		return;
	}

	StringSetFromArray(CLASS_KEY.data(), m_ClassBlacklist, val);
	StringSetFromArray(TITLE_KEY.data(), m_TitleBlacklist, val);
	StringSetFromArray(FILE_KEY.data(), m_FileBlacklist, val);
}

bool Blacklist::IsBlacklisted(Window window) const
{
	// This is the fastest because we do the less string manipulation, so always try it first
	if (!m_ClassBlacklist.empty())
	{
		// TODO: Remove wstring copy when transparent comparators land
		if (m_ClassBlacklist.contains(std::wstring(window.classname())))
		{
			return true;
		}
	}

	if (!m_FileBlacklist.empty())
	{
		if (m_FileBlacklist.contains(window.file().filename()))
		{
			return true;
		}
	}

	// Do it last because titles can change, so it's less reliable.
	if (!m_TitleBlacklist.empty())
	{
		for (const std::wstring &value : m_TitleBlacklist)
		{
			if (window.title().find(value) != std::wstring_view::npos)
			{
				return true;
			}
		}
	}

	return false;
}