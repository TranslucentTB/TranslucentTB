#pragma once
#include <rapidjson/document.h>
#include <rapidjson/encodings.h>
#include <string>
#include <string_view>
#include <unordered_set>

#include "../win32.hpp"

#ifdef _TRANSLUCENTTB_EXE
#include "../../TranslucentTB/windows/window.hpp"
#include "../../ProgramLog/error/std.hpp"
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

	inline void Deserialize(const RapidJSONHelper::value_t &obj)
	{
		DeserializeStringSet(obj, m_ClassList, CLASS_KEY);
		DeserializeStringSet(obj, m_TitleList, TITLE_KEY);
		DeserializeStringSet(obj, m_FileList, FILE_KEY);
	}

#ifdef _TRANSLUCENTTB_EXE
	inline bool IsFiltered(Window window) const
	{
		// This is the fastest because we do the less string manipulation, so always try it first
		if (!m_ClassList.empty())
		{
			if (const auto className = window.classname())
			{
				if (m_ClassList.contains(*className))
				{
					return true;
				}
			}
			else
			{
				return false;
			}
		}

		if (!m_FileList.empty())
		{
			if (const auto file = window.file())
			{
				try
				{
					if (m_FileList.contains(file->filename().native()))
					{
						return true;
					}
				}
				StdSystemErrorCatch(spdlog::level::warn, L"Failed to check if window process is part of window filter");
			}
			else
			{
				return false;
			}
		}

		// Do it last because titles can change, so it's less reliable.
		if (!m_TitleList.empty())
		{
			if (const auto title = window.title())
			{
				for (const auto &value : m_TitleList)
				{
					if (title->find(value) != std::wstring::npos)
					{
						return true;
					}
				}
			}
		}

		return false;
	}
#endif

private:
	template<typename Writer, typename Hash, typename Equal, typename Alloc>
	inline static void SerializeStringSet(Writer &writer, const std::unordered_set<std::wstring, Hash, Equal, Alloc> &set, std::wstring_view key)
	{
		RapidJSONHelper::WriteKey(writer, key);
		writer.StartArray();
		for (const std::wstring &str : set)
		{
			RapidJSONHelper::WriteString(writer, str);
		}
		writer.EndArray();
	}

	template<typename Hash, typename Equal, typename Alloc>
	inline static void DeserializeStringSet(const RapidJSONHelper::value_t &obj, std::unordered_set<std::wstring, Hash, Equal, Alloc> &set, std::wstring_view key)
	{
		if (const auto it = obj.FindMember(RapidJSONHelper::StringViewToValue(key)); it != obj.MemberEnd())
		{
			const auto &array = it->value;
			RapidJSONHelper::EnsureType(rapidjson::Type::kArrayType, array.GetType(), key);

			for (const auto &elem : array.GetArray())
			{
				RapidJSONHelper::EnsureType(rapidjson::Type::kStringType, elem.GetType(), L"array element");
				set.emplace(RapidJSONHelper::ValueToStringView(elem));
			}
		}
	}

	std::unordered_set<std::wstring> m_ClassList;
	std::unordered_set<std::wstring> m_TitleList;
	win32::FilenameSet m_FileList;

	static constexpr std::wstring_view CLASS_KEY = L"window_class";
	static constexpr std::wstring_view TITLE_KEY = L"window_title";
	static constexpr std::wstring_view FILE_KEY = L"process_name";
};
