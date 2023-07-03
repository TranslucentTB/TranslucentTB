#pragma once
#include <rapidjson/document.h>
#include <rapidjson/encodings.h>
#include <string>
#include <string_view>
#include <unordered_set>

#include "rapidjsonhelper.hpp"
#include "../win32.hpp"
#include "../constants.hpp"

#ifdef _TRANSLUCENTTB_EXE
#include "../../TranslucentTB/windows/window.hpp"
#include "../../ProgramLog/error/std.hpp"
#endif

struct WindowFilter {
	std::unordered_set<std::wstring> ClassList;
	std::unordered_set<std::wstring> TitleList;
	win32::FilenameSet FileList;

	WindowFilter() = default;
	WindowFilter(std::unordered_set<std::wstring> classList, std::unordered_set<std::wstring> titleList, win32::FilenameSet fileList) :
		ClassList(std::move(classList)),
		TitleList(std::move(titleList)),
		FileList(std::move(fileList))
	{ }

	template<class Writer>
	inline void Serialize(Writer &writer) const
	{
		SerializeStringSet(writer, ClassList, CLASS_KEY);
		SerializeStringSet(writer, TitleList, TITLE_KEY);
		SerializeStringSet(writer, FileList, FILE_KEY);
	}

	inline void Deserialize(const rjh::value_t &obj, void (*unknownKeyCallback)(std::wstring_view))
	{
		rjh::EnsureType(rj::Type::kObjectType, obj.GetType(), L"root node");

		for (auto it = obj.MemberBegin(); it != obj.MemberEnd(); ++it)
		{
			rjh::EnsureType(rj::Type::kStringType, it->name.GetType(), L"member name");

			const auto key = rjh::ValueToStringView(it->name);
			if (key == CLASS_KEY)
			{
				DeserializeStringSet(it->value, ClassList, key);
			}
			else if (key == TITLE_KEY)
			{
				DeserializeStringSet(it->value, TitleList, key);
			}
			else if (key == FILE_KEY)
			{
				DeserializeStringSet(it->value, FileList, key);
			}
			else if (unknownKeyCallback)
			{
				unknownKeyCallback(key);
			}
		}
	}

#ifdef _TRANSLUCENTTB_EXE
	inline bool IsFiltered(Window window) const
	{
		// TODO: add logging
		// This is the fastest because we do the less string manipulation, so always try it first
		if (!ClassList.empty())
		{
			if (const auto className = window.classname())
			{
				if (ClassList.contains(*className))
				{
					return true;
				}
			}
			else
			{
				return false;
			}
		}

		if (!FileList.empty())
		{
			if (const auto file = window.file())
			{
				try
				{
					if (FileList.contains(file->filename().native()))
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
		if (!TitleList.empty())
		{
			if (const auto title = window.title())
			{
				for (const auto &value : TitleList)
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
		rjh::WriteKey(writer, key);
		writer.StartArray();
		for (const std::wstring &str : set)
		{
			rjh::WriteString(writer, str);
		}
		writer.EndArray();
	}

	template<typename Hash, typename Equal, typename Alloc>
	inline static void DeserializeStringSet(const rjh::value_t &arr, std::unordered_set<std::wstring, Hash, Equal, Alloc> &set, std::wstring_view key)
	{
		rjh::EnsureType(rj::Type::kArrayType, arr.GetType(), key);

		for (const auto &elem : arr.GetArray())
		{
			rjh::EnsureType(rj::Type::kStringType, elem.GetType(), L"array element");
			set.emplace(rjh::ValueToStringView(elem));
		}
	}
};
