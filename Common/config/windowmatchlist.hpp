#pragma once
#include <rapidjson/document.h>
#include <rapidjson/encodings.h>
#include <string>
#include <string_view>
#include <unordered_set>
#include <wil/safecast.h>

#include "../util/strings.hpp"

#ifdef _TRANSLUCENTTB_EXE
#include "../window.hpp"
#endif

class WindowMatchList {
public:
	template<class Writer>
	inline void Serialize(Writer &writer) const
	{
		SerializeStringSet(writer, m_ClassList, CLASS_KEY);
		SerializeStringSet(writer, m_TitleList, TITLE_KEY);
		SerializeStringSet(writer, m_FileList, FILE_KEY);
	}

	inline void Deserialize(const rapidjson::GenericValue<rapidjson::UTF16LE<>> &val)
	{
		if (!val.IsObject())
		{
			return;
		}

		DeserializeStringSet(val, m_ClassList, CLASS_KEY);
		DeserializeStringSet(val, m_TitleList, TITLE_KEY);
		DeserializeStringSet(val, m_FileList, FILE_KEY);
	}

#ifdef _TRANSLUCENTTB_EXE
	inline bool Matches(Window window) const
	{
		// This is the fastest because we do the less string manipulation, so always try it first
		if (!m_ClassList.empty())
		{
			// TODO: Remove wstring copy when transparent comparators land
			if (m_ClassList.contains(std::wstring(window.classname())))
			{
				return true;
			}
		}

		if (!m_FileList.empty())
		{
			if (m_FileList.contains(window.file().filename()))
			{
				return true;
			}
		}

		// Do it last because titles can change, so it's less reliable.
		if (!m_TitleList.empty())
		{
			for (const std::wstring &value : m_TitleList)
			{
				if (window.title().find(value) != std::wstring_view::npos)
				{
					return true;
				}
			}
		}

		return false;
	}
#endif

private:
	template<typename Writer, typename T>
	inline static void SerializeStringSet(Writer &writer, const T &set, std::wstring_view key)
	{
		writer.String(key.data(), wil::safe_cast<rapidjson::SizeType>(key.length()));
		writer.StartArray();
		for (const std::wstring &str : set)
		{
			writer.String(str);
		}
		writer.EndArray();
	}

	template<typename T>
	inline static void DeserializeStringSet(const rapidjson::GenericValue<rapidjson::UTF16LE<>> &val, T &set, std::wstring_view key)
	{
		if (const auto arr = val.FindMember(key.data()); arr != val.MemberEnd() && arr->value.IsArray())
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

	std::unordered_set<std::wstring> m_ClassList;
	std::unordered_set<std::wstring> m_TitleList;
	Util::string_set m_FileList;

	static constexpr std::wstring_view CLASS_KEY = L"window_class";
	static constexpr std::wstring_view TITLE_KEY = L"window_title";
	static constexpr std::wstring_view FILE_KEY = L"process_file";
};