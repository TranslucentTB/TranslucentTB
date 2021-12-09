#pragma once
#include "../arch.h"
#include <array>
#include <rapidjson/document.h>
#include <rapidjson/encodings.h>
#include <string_view>
#include <windef.h>

#if __has_include(<winrt/TranslucentTB.Xaml.Models.Primitives.h>)
#define HAS_WINRT_CONFIG
#include "../winrt.hpp"
#include <winrt/TranslucentTB.Xaml.Models.Primitives.h>
#endif

#include "rapidjsonhelper.hpp"
#include "../undoc/user32.hpp"
#include "../util/color.hpp"

struct TaskbarAppearance {
	ACCENT_STATE Accent = ACCENT_NORMAL;
	Util::Color Color = { 0, 0, 0, 0 };
	bool ShowPeek = true;

	constexpr TaskbarAppearance() noexcept = default;
	constexpr TaskbarAppearance(ACCENT_STATE accent, Util::Color color, bool showPeek) noexcept :
		Accent(accent),
		Color(color),
		ShowPeek(showPeek)
	{ }

	template<class Writer>
	inline void Serialize(Writer &writer) const
	{
		rjh::Serialize(writer, Accent, ACCENT_KEY, ACCENT_MAP);
		rjh::Serialize(writer, Color.ToString(), COLOR_KEY);
		rjh::Serialize(writer, ShowPeek, SHOW_PEEK_KEY);
	}

	void Deserialize(const rjh::value_t &obj, void (*unknownKeyCallback)(std::wstring_view))
	{
		rjh::EnsureType(rj::Type::kObjectType, obj.GetType(), L"root node");

		for (auto it = obj.MemberBegin(); it != obj.MemberEnd(); ++it)
		{
			rjh::EnsureType(rj::Type::kStringType, it->name.GetType(), L"member name");

			InnerDeserialize(rjh::ValueToStringView(it->name), it->value, unknownKeyCallback);
		}
	}

#ifdef HAS_WINRT_CONFIG
	TaskbarAppearance(const txmp::TaskbarAppearance &winrtObj) noexcept :
		Accent(static_cast<ACCENT_STATE>(winrtObj.Accent())),
		Color(winrtObj.Color()),
		ShowPeek(winrtObj.ShowPeek())
	{ }

	operator txmp::TaskbarAppearance() const
	{
		return { static_cast<txmp::AccentState>(Accent), Color, ShowPeek };
	}
#endif

protected:
	void InnerDeserialize(std::wstring_view key, const rjh::value_t &val, void (*unknownKeyCallback)(std::wstring_view))
	{
		if (key == ACCENT_KEY)
		{
			rjh::Deserialize(val, Accent, key, ACCENT_MAP);
		}
		else if (key == COLOR_KEY)
		{
			rjh::EnsureType(rj::Type::kStringType, val.GetType(), key);

			const auto colorStr = rjh::ValueToStringView(val);
			try
			{
				Color = Util::Color::FromString(colorStr);
			}
			catch (...)
			{
				throw rjh::DeserializationError {
					std::format(L"Found invalid string \"{}\" while deserializing {}", colorStr, key)
				};
			}
		}
		else if (key == SHOW_PEEK_KEY)
		{
			rjh::Deserialize(val, ShowPeek, key);
		}
		else if (unknownKeyCallback)
		{
			unknownKeyCallback(key);
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
	static constexpr std::wstring_view SHOW_PEEK_KEY = L"show_peek";
};
