#pragma once
#include <rapidjson/document.h>
#include <rapidjson/encodings.h>
#include <string_view>
#include <unordered_map>

#include "util/others.hpp"

namespace RapidJSONHelper {
	template<class Writer>
	inline void Serialize(Writer &writer, bool value, std::wstring_view key)
	{
		writer.String(key.data(), key.length());
		writer.Bool(value);
	}

	template<class Writer, class T>
	inline void Serialize(Writer &writer, const T &member, std::wstring_view key, const std::unordered_map<T, std::wstring_view> &map)
	{
		writer.String(key.data(), key.length());
		const auto enum_str = Util::FindOrDefault(map, member);
		writer.String(enum_str.data(), enum_str.length());
	}

	template<class Writer, class T>
	inline void Serialize(Writer &writer, const T &member, std::wstring_view key)
	{
		writer.String(key.data(), key.length());
		writer.StartObject();
		member.Serialize(writer);
		writer.EndObject();
	}

	inline void Deserialize(const rapidjson::GenericValue<rapidjson::UTF16LE<>> &val, bool &value, std::wstring_view key)
	{
		if (!val.IsObject())
		{
			return;
		}

		if (const auto member = val.FindMember(key.data()); member != val.MemberEnd() && member->value.IsBool())
		{
			value = member->value.GetBool();
		}
	}

	template<class T>
	inline void Deserialize(const rapidjson::GenericValue<rapidjson::UTF16LE<>> & val, T &member, std::wstring_view key, const std::unordered_map<T, std::wstring_view> &map)
	{
		if (!val.IsObject())
		{
			return;
		}

		if (const auto value = val.FindMember(key.data()); value != val.MemberEnd() && value->value.IsString())
		{
			if (const auto iter = Util::FindValue(map, { value->value.GetString(), value->value.GetStringLength() }); iter != map.end())
			{
				member = iter->first;
			}
		}
	}

	template<class T>
	inline void Deserialize(const rapidjson::GenericValue<rapidjson::UTF16LE<>> &val, T &member, std::wstring_view key)
	{
		if (!val.IsObject())
		{
			return;
		}

		if (const auto value = val.FindMember(key.data()); value != val.MemberEnd())
		{
			member.Deserialize(value->value);
		}
	}
}