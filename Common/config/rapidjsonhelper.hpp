#pragma once
#include <algorithm>
#include <array>
#include <rapidjson/document.h>
#include <rapidjson/encodings.h>
#include <string_view>

#include "../util/null_terminated_string_view.hpp"
#include "../util/map.hpp"

namespace RapidJSONHelper {
	template<class Writer>
	inline void Serialize(Writer &writer, bool value, std::wstring_view key)
	{
		writer.Key(key.data(), static_cast<rapidjson::SizeType>(key.length()));
		writer.Bool(value);
	}

	template<class Writer, class T, std::size_t size>
	inline void Serialize(Writer &writer, const T &member, std::wstring_view key, const std::array<std::wstring_view, size> &arr)
	{
		if (member >= 0 && member <= size - 1)
		{
			writer.Key(key.data(), static_cast<rapidjson::SizeType>(key.length()));

			const auto enum_str = arr[member];
			writer.String(enum_str.data(), static_cast<rapidjson::SizeType>(enum_str.length()));
		}
	}

	template<class Writer, class T>
	inline void Serialize(Writer &writer, const T &member, std::wstring_view key)
	{
		writer.Key(key.data(), static_cast<rapidjson::SizeType>(key.length()));
		writer.StartObject();
		member.Serialize(writer);
		writer.EndObject();
	}

	inline void Deserialize(const rapidjson::GenericValue<rapidjson::UTF16LE<>> &val, bool &value, Util::null_terminated_wstring_view key)
	{
		if (!val.IsObject())
		{
			return;
		}

		if (const auto member = val.FindMember(key.c_str()); member != val.MemberEnd() && member->value.IsBool())
		{
			value = member->value.GetBool();
		}
	}

	template<class T, std::size_t size>
	inline void Deserialize(const rapidjson::GenericValue<rapidjson::UTF16LE<>> &val, T &member, Util::null_terminated_wstring_view key, const std::array<std::wstring_view, size> &arr)
	{
		if (!val.IsObject())
		{
			return;
		}

		if (const auto value = val.FindMember(key.c_str()); value != val.MemberEnd() && value->value.IsString())
		{
			if (const auto it = std::find(arr.begin(), arr.end(), std::wstring_view(value->value.GetString(), value->value.GetStringLength()));
				it != arr.end())
			{
				member = static_cast<T>(it - arr.begin());
			}
		}
	}

	template<class T>
	inline void Deserialize(const rapidjson::GenericValue<rapidjson::UTF16LE<>> &val, T &member, Util::null_terminated_wstring_view key)
	{
		if (!val.IsObject())
		{
			return;
		}

		if (const auto value = val.FindMember(key.c_str()); value != val.MemberEnd())
		{
			member.Deserialize(value->value);
		}
	}
}
