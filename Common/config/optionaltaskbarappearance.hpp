#pragma once
#include <string_view>

#include "rapidjsonhelper.hpp"
#include "taskbarappearance.hpp"

struct OptionalTaskbarAppearance : TaskbarAppearance {
	bool Enabled;

	template <typename Writer>
	inline void Serialize(Writer &writer) const
	{
		RapidJSONHelper::Serialize(writer, Enabled, ENABLED_KEY);
		TaskbarAppearance::Serialize(writer);
	}

	void Deserialize(const rapidjson::GenericValue<rapidjson::UTF16LE<>> &obj)
	{
		RapidJSONHelper::Deserialize(obj, Enabled, ENABLED_KEY);
		TaskbarAppearance::Deserialize(obj);
	}

private:
	static constexpr std::wstring_view ENABLED_KEY = L"enabled";
};
