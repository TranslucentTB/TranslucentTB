#pragma once
#include "../arch.h"
#include <array>
#include <rapidjson/document.h>
#include <rapidjson/encodings.h>
#include <string_view>
#include <windef.h>

#include "rapidjsonhelper.hpp"
#include "../undoc/user32.hpp"
#include "../util/colors.hpp"
#include "../util/null_terminated_string_view.hpp"

struct TaskbarAppearance {
	ACCENT_STATE Accent;
	COLORREF     Color;

	template<class Writer>
	inline void Serialize(Writer &writer) const
	{
		RapidJSONHelper::Serialize(writer, Accent, ACCENT_KEY, ACCENT_MAP);

		writer.String(COLOR_KEY.data(), static_cast<rapidjson::SizeType>(COLOR_KEY.length()));
		writer.String(Util::StringFromColor(Util::SwapColorEndian(Color)));

		writer.String(OPACITY_KEY.data(), static_cast<rapidjson::SizeType>(OPACITY_KEY.length()));
		writer.Uint((Color & 0xFF000000) >> 24);
	}

	void Deserialize(const rapidjson::GenericValue<rapidjson::UTF16LE<>> &obj)
	{
		RapidJSONHelper::Deserialize(obj, Accent, ACCENT_KEY, ACCENT_MAP);

		if (const auto it = obj.FindMember(COLOR_KEY.data()); it != obj.MemberEnd())
		{
			const auto &color = it->value;
			RapidJSONHelper::EnsureType(rapidjson::Type::kStringType, color.GetType(), COLOR_KEY);

			Color = (Color & 0xFF000000) + Util::SwapColorEndian(Util::ColorFromString({ color.GetString(), color.GetStringLength() }));
		}

		if (const auto it = obj.FindMember(OPACITY_KEY.data()); it != obj.MemberEnd())
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

	static constexpr Util::null_terminated_wstring_view ACCENT_KEY = L"accent";
	static constexpr Util::null_terminated_wstring_view COLOR_KEY = L"color";
	static constexpr Util::null_terminated_wstring_view OPACITY_KEY = L"opacity";
};
