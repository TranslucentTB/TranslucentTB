#pragma once
#include <rapidjson/document.h>
#include <rapidjson/encodings.h>
#include <string>
#include <string_view>
#include <unordered_set>

#include "../util/null_terminated_string_view.hpp"
#include "../win32.hpp"

#ifdef _TRANSLUCENTTB_EXE
#include "../window.hpp"
#endif

class WindowFilter {
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
	inline bool IsFiltered(Window window) const
	{
		// This is the fastest because we do the less string manipulation, so always try it first
		if (!m_ClassList.empty() && m_ClassList.contains(window.classname()))
		{
			return true;
		}

		if (!m_FileList.empty() && m_FileList.contains(window.file().filename().native()))
		{
			return true;
		}

		// Do it last because titles can change, so it's less reliable.
		if (!m_TitleList.empty())
		{
			const std::wstring title = window.title();
			for (const std::wstring &value : m_TitleList)
			{
				if (title.find(value) != std::wstring::npos)
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
		writer.String(key.data(), static_cast<rapidjson::SizeType>(key.length()));
		writer.StartArray();
		for (const std::wstring &str : set)
		{
			writer.String(str);
		}
		writer.EndArray();
	}

	template<typename T>
	inline static void DeserializeStringSet(const rapidjson::GenericValue<rapidjson::UTF16LE<>> &val, T &set, Util::null_terminated_wstring_view key)
	{
		if (const auto arr = val.FindMember(key.c_str()); arr != val.MemberEnd() && arr->value.IsArray())
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
	win32::FilenameSet m_FileList;

	static constexpr Util::null_terminated_wstring_view CLASS_KEY = L"window_class";
	static constexpr Util::null_terminated_wstring_view TITLE_KEY = L"window_title";
	static constexpr Util::null_terminated_wstring_view FILE_KEY = L"process_file";
};
