#pragma once
#include <string_view>
#include <vector>
#include <format>

#include "rapidjsonhelper.hpp"
#include "optionaltaskbarappearance.hpp"
#include "rule.hpp"
#include "../../TranslucentTB/windows/window.hpp"

struct RuledTaskbarAppearance : OptionalTaskbarAppearance {
	std::vector<Rule> Rules = {};

	constexpr RuledTaskbarAppearance() noexcept = default;
	constexpr RuledTaskbarAppearance(std::vector<Rule> rules, bool enabled, ACCENT_STATE accent, Util::Color color, bool showPeek) :
		Rules(rules),
		OptionalTaskbarAppearance(enabled, accent, color, showPeek)
	{ }

	template<typename Writer>
	inline void Serialize(Writer& writer) const
	{
		OptionalTaskbarAppearance::Serialize(writer);
		SerializeRulesVec(writer, Rules, RULES_KEY);
	}

	void Deserialize(const rjh::value_t& obj, void (*unknownKeyCallback)(std::wstring_view))
	{
		rjh::EnsureType(rj::Type::kObjectType, obj.GetType(), L"root node");

		for (auto it = obj.MemberBegin(); it != obj.MemberEnd(); ++it)
		{
			rjh::EnsureType(rj::Type::kStringType, it->name.GetType(), L"member name");

			const auto key = rjh::ValueToStringView(it->name);
			if (key == RULES_KEY)
			{
				DeserializeRulesVec(it->value, Rules, key, unknownKeyCallback);
			}
			else
			{
				OptionalInnerDeserialize(key, it->value, unknownKeyCallback);
			}
		}
	}

	inline const std::optional<Rule> FindRule(const Window window) const
	{

		for (const Rule &rule : Rules)
		{
			// This is the fastest because we do the less string manipulation, so always try it first
			if (!rule.WindowClass.empty())
			{

				if (rule.WindowClass == window.classname())
				{
					return rule;
				}

			}

			if (!rule.ProcessName.empty())
			{

				if (rule.ProcessName == window.file()->filename().native())
				{
					return rule;
				}

			}

			// Do it last because titles can change, so it's less reliable.
			if (!rule.WindowTitle.empty())
			{

				if (rule.WindowTitle == window.title())
				{
					return rule;
				}

			}
		}

		return std::nullopt;
	}

private:
	template <typename Writer>
	inline static void SerializeRulesVec(Writer& writer, const std::vector<Rule>& vec, std::wstring_view key)
	{
		rjh::WriteKey(writer, key);
		writer.StartArray();
		for (const Rule &rule : vec)
		{
			writer.StartObject();
			rule.Serialize(writer);
			writer.EndObject();
		}
		writer.EndArray();
	}

	inline static void DeserializeRulesVec(const rjh::value_t& arr, std::vector<Rule>& vec, std::wstring_view key, void (*unknownKeyCallback)(std::wstring_view))
	{
		rjh::EnsureType(rj::Type::kArrayType, arr.GetType(), key);

		for (const auto &elem : arr.GetArray())
		{
			rjh::EnsureType(rj::Type::kObjectType, elem.GetType(), L"array element");
			Rule rule;
			rule.Deserialize(elem, unknownKeyCallback);
			vec.push_back(rule);
		}
	}

	static constexpr std::wstring_view RULES_KEY = L"rules";
};
