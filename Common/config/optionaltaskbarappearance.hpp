#pragma once
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

	void Deserialize(const rapidjson::GenericValue<rapidjson::UTF16LE<>> &val)
	{
		if (!val.IsObject())
		{
			return;
		}

		RapidJSONHelper::Deserialize(val, Enabled, ENABLED_KEY);
		TaskbarAppearance::Deserialize(val);
	}

private:
	static constexpr std::wstring_view ENABLED_KEY = L"enabled";
};
