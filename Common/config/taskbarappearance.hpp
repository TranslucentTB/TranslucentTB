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
#include "../win32.hpp"

struct TaskbarAppearance {
	inline static bool IsBlurSupported()
	{
		static const bool isBlurSupported = []
		{
			if (win32::IsExactBuild(22000))
			{
				// Windows 11 RTM. sometimes very laggy at release, fixed in KB5006746 (22000.282)
				if (const auto [version, hr] = win32::GetWindowsBuild(); SUCCEEDED(hr))
				{
					return version.Revision >= 282;
				}
				else
				{
					return false;
				}
			}
			else
			{
				// always works in Windows 10, but broken in Windows 11 besides RTM with KB5006746.
				// since we check IsExactBuild above, using an inverted IsAtLeastBuild here 
				// makes sure we return false for future versions of Windows 11 as well.
				return !win32::IsAtLeastBuild(22000);
			}
		}();

		return isBlurSupported;
	}

	ACCENT_STATE Accent = ACCENT_NORMAL;
	Util::Color Color = { 0, 0, 0, 0 };
	bool ShowPeek = true;
	bool ShowLine = true;

	constexpr TaskbarAppearance() noexcept = default;
	constexpr TaskbarAppearance(ACCENT_STATE accent, Util::Color color, bool showPeek, bool showLine) noexcept :
		Accent(accent),
		Color(color),
		ShowPeek(showPeek),
		ShowLine(showLine)
	{ }

	template<class Writer>
	inline void Serialize(Writer &writer) const
	{
		rjh::Serialize(writer, Accent, ACCENT_KEY, ACCENT_MAP);
		rjh::Serialize(writer, Color.ToString(), COLOR_KEY);
		rjh::Serialize(writer, ShowPeek, SHOW_PEEK_KEY);
		rjh::Serialize(writer, ShowLine, SHOW_LINE_KEY);
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
		ShowPeek(winrtObj.ShowPeek()),
		ShowLine(winrtObj.ShowLine())
	{ }

	operator txmp::TaskbarAppearance() const
	{
		return { static_cast<txmp::AccentState>(Accent), Color, ShowPeek, ShowLine };
	}
#endif

protected:
	void InnerDeserialize(std::wstring_view key, const rjh::value_t &val, void (*unknownKeyCallback)(std::wstring_view))
	{
		if (key == ACCENT_KEY)
		{
			rjh::Deserialize(val, Accent, key, ACCENT_MAP);

			// when blur is broken upgrade people to acrylic
			if (Accent == ACCENT_ENABLE_BLURBEHIND && !IsBlurSupported())
			{
				Accent = ACCENT_ENABLE_ACRYLICBLURBEHIND;
			}
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
		else if (key == SHOW_LINE_KEY)
		{
			rjh::Deserialize(val, ShowLine, key);
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
	static constexpr std::wstring_view SHOW_LINE_KEY = L"show_line";
};
