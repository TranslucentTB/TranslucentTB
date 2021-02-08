#pragma once
#include <string_view>

#include "rapidjsonhelper.hpp"
#include "taskbarappearance.hpp"

struct OptionalTaskbarAppearance : TaskbarAppearance {
	bool Enabled;

	template <typename Writer>
	inline void Serialize(Writer &writer) const
	{
		rjh::Serialize(writer, Enabled, ENABLED_KEY);
		TaskbarAppearance::Serialize(writer);
	}

	void Deserialize(const rjh::value_t &obj, void (*unknownKeyCallback)(std::wstring_view))
	{
		rjh::EnsureType(rj::Type::kObjectType, obj.GetType(), L"root node");

		for (auto it = obj.MemberBegin(); it != obj.MemberEnd(); ++it)
		{
			rjh::EnsureType(rj::Type::kStringType, it->name.GetType(), L"member name");

			const auto key = rjh::ValueToStringView(it->name);
			if (key == ENABLED_KEY)
			{
				rjh::Deserialize(it->value, Enabled, key);
			}
			else
			{
				InnerDeserialize(key, it->value, unknownKeyCallback);
			}
		}
	}

private:
	static constexpr std::wstring_view ENABLED_KEY = L"enabled";
};
