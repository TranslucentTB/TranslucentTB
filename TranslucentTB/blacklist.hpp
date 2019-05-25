#pragma once
#include <rapidjson/document.h>
#include <rapidjson/encodings.h>
#include <string>
#include <unordered_set>

#include "util/strings.hpp"
#include "windows/window.hpp"

class Config;

class Blacklist {
public:
	template<class Writer>
	inline void Serialize(Writer &writer) const
	{
		writer.String(L"window_class");
		writer.StartArray();
		for (const std::wstring &str : m_ClassBlacklist)
		{
			writer.String(str);
		}
		writer.EndArray();

		writer.String(L"window_title");
		writer.StartArray();
		for (const std::wstring &str : m_TitleBlacklist)
		{
			writer.String(str);
		}
		writer.EndArray();

		writer.String(L"process_file");
		writer.StartArray();
		for (const std::wstring &str : m_FileBlacklist)
		{
			writer.String(str);
		}
		writer.EndArray();
	}

	void Deserialize(const rapidjson::GenericValue<rapidjson::UTF16LE<>> &val);

	bool IsBlacklisted(Window window) const;

private:
	template<typename T>
	inline void StringSetFromArray(const std::wstring &name, T &set, const rapidjson::GenericValue<rapidjson::UTF16LE<>> &val)
	{
		if (const auto arr = val.FindMember(name); arr != val.MemberEnd() && arr->value.IsArray())
		{
			for (const auto &elem : arr->value.GetArray())
			{
				if (elem.IsString())
				{
					set.emplace(elem.GetString(), elem.GetStringLength());
				}
			}
		}
	}

	std::unordered_set<std::wstring> m_ClassBlacklist;
	std::unordered_set<std::wstring> m_TitleBlacklist;
	Util::string_set m_FileBlacklist;
};