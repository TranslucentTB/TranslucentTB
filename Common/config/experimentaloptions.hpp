#pragma once
#include "../arch.h"
#include <optional>
#include <string_view>

#include "rapidjsonhelper.hpp"

struct ExperimentalOptions {
	std::optional<bool> UseXamlContextMenu;

	template<class Writer>
	inline void Serialize(Writer &writer) const
	{
		rjh::Serialize(writer, UseXamlContextMenu, USE_XAML_CONTEXT_MENU_KEY);
	}

	void Deserialize(const rjh::value_t &obj, void (*unknownKeyCallback)(std::wstring_view))
	{
		rjh::EnsureType(rj::Type::kObjectType, obj.GetType(), L"root node");

		for (auto it = obj.MemberBegin(); it != obj.MemberEnd(); ++it)
		{
			rjh::EnsureType(rj::Type::kStringType, it->name.GetType(), L"member name");

			const auto key = rjh::ValueToStringView(it->name);
			if (key == USE_XAML_CONTEXT_MENU_KEY)
			{
				rjh::Deserialize(it->value, UseXamlContextMenu, key);
			}
			else if (unknownKeyCallback)
			{
				unknownKeyCallback(key);
			}
		}
	}

private:
	static constexpr std::wstring_view USE_XAML_CONTEXT_MENU_KEY = L"use_xaml_context_menu";
};
