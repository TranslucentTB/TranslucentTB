#pragma once
#include <rapidjson/document.h>
#include <rapidjson/encodings.h>
#include <string>
#include <string_view>
#include <unordered_set>

#include "util/strings.hpp"
#include "windows/window.hpp"

class Config;

class Blacklist {
public:
	template<class Writer>
	inline void Serialize(Writer &writer) const
	{
		writer.String(CLASS_KEY.data(), CLASS_KEY.length());
		writer.StartArray();
		for (const std::wstring &str : m_ClassBlacklist)
		{
			writer.String(str);
		}
		writer.EndArray();

		writer.String(TITLE_KEY.data(), TITLE_KEY.length());
		writer.StartArray();
		for (const std::wstring &str : m_TitleBlacklist)
		{
			writer.String(str);
		}
		writer.EndArray();

		writer.String(FILE_KEY.data(), FILE_KEY.length());
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
	inline void StringSetFromArray(std::wstring_view name, T &set, const rapidjson::GenericValue<rapidjson::UTF16LE<>> &val)
	{
		if (const auto arr = val.FindMember(name.data()); arr != val.MemberEnd() && arr->value.IsArray())
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

	static constexpr std::wstring_view CLASS_KEY = L"window_class";
	static constexpr std::wstring_view TITLE_KEY = L"window_title";
	static constexpr std::wstring_view FILE_KEY = L"process_file";
};