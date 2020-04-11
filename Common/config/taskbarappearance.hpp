#pragma once
#include "../arch.h"
#include <array>
#include <fmt/format.h>
#include <rapidjson/document.h>
#include <rapidjson/encodings.h>
#include <string_view>
#include <windef.h>

#include "rapidjsonhelper.hpp"
#include "../undoc/user32.hpp"
#include "../util/colors.hpp"
#include "../util/to_string_view.hpp"

struct TaskbarAppearance {
	ACCENT_STATE Accent;
	COLORREF     Color;

	template<class Writer>
	inline void Serialize(Writer &writer) const
	{
		RapidJSONHelper::Serialize(writer, Accent, ACCENT_KEY, ACCENT_MAP);

		RapidJSONHelper::WriteKey(writer, COLOR_KEY);
		fmt::wmemory_buffer buf;
		Util::StringFromColor(buf, Util::SwapColorEndian(Color));
		RapidJSONHelper::WriteString(writer, Util::ToStringView(buf));

		RapidJSONHelper::WriteKey(writer, OPACITY_KEY);
		writer.Uint((Color & 0xFF000000) >> 24);
	}

	void Deserialize(const rapidjson::GenericValue<rapidjson::UTF16LE<>> &obj)
	{
		RapidJSONHelper::Deserialize(obj, Accent, ACCENT_KEY, ACCENT_MAP);

		if (const auto it = obj.FindMember(RapidJSONHelper::StringViewToValue(COLOR_KEY)); it != obj.MemberEnd())
		{
			const auto &color = it->value;
			RapidJSONHelper::EnsureType(rapidjson::Type::kStringType, color.GetType(), COLOR_KEY);

			const auto colorStr = RapidJSONHelper::ValueToStringView(color);
			try
			{
				Color = (Color & 0xFF000000) + Util::SwapColorEndian(Util::ColorFromString(colorStr));
			}
			catch (...)
			{
				throw RapidJSONHelper::DeserializationError {
					fmt::format(fmt(L"Found invalid color string \"{}\" while deserializing key \"{}\""), colorStr, COLOR_KEY)
				};
			}
		}

		if (const auto it = obj.FindMember(RapidJSONHelper::StringViewToValue(OPACITY_KEY)); it != obj.MemberEnd())
		{
			const auto &opacity = it->value;
			RapidJSONHelper::EnsureType(rapidjson::Type::kNumberType, opacity.GetType(), OPACITY_KEY);

			Color = (std::clamp(opacity.GetInt(), 0, 255) << 24) + (Color & 0xFFFFFF);
		}
	}

private:
	static constexpr std::array<std::wstring_view, 5> ACCENT_MAP = {
		L"normal",
		L"opaque",
		L"clear",
		L"blur",
		L"acrylic"
	};

	static constexpr std::wstring_view ACCENT_KEY = L"accent";
	static constexpr std::wstring_view COLOR_KEY = L"color";
	static constexpr std::wstring_view OPACITY_KEY = L"opacity";
};
