#pragma once
#include <string_view>
#include <vector>

#include "rapidjsonhelper.hpp"
#include "optionaltaskbarappearance.hpp"
#include "rule.hpp"
#include "../../TranslucentTB/windows/window.hpp"

struct RuledTaskbarAppearance : OptionalTaskbarAppearance {
	std::vector<Rule> m_Rules = {};

	RuledTaskbarAppearance() noexcept = default;
	RuledTaskbarAppearance(bool enabled, ACCENT_STATE accent, Util::Color color, bool showPeek) noexcept :
		OptionalTaskbarAppearance(enabled, accent, color, showPeek)
	{ }

	template<typename Writer>
	inline void Serialize(Writer& writer) const
	{
		SerializeRulesVec(writer, m_Rules, RULES_KEY);
		OptionalTaskbarAppearance::Serialize(writer);
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
				DeserializeRulesVec(it->value, m_Rules, key, unknownKeyCallback);
			}
			else
			{
				OptionalInnerDeserialize(key, it->value, unknownKeyCallback);
			}
		}
	}

	inline const Rule* FindRule(Window window)
	{
		for (const Rule& rule : m_Rules)
		{
			// This is the fastest because we do the less string manipulation, so always try it first
			if (!rule.m_Class.empty())
			{
				if (const auto className = window.classname())
				{
					if (rule.m_Class == *className)
					{
						return &rule;
					}
				}
			}

			if (!rule.m_ProcessName.empty())
			{
				if (const auto file = window.file())
				{
					if (rule.m_ProcessName == file->filename().native())
					{
						return &rule;
					}
				}
			}

			// Do it last because titles can change, so it's less reliable.
			if (!rule.m_Title.empty())
			{
				if (const auto title = window.title())
				{
					if (rule.m_Title== title)
					{
						return &rule;
					}
				}
			}
		}

		return nullptr;
	}

private:
	template <typename Writer>
	inline static void SerializeRulesVec(Writer& writer, const std::vector<Rule>& vec, std::wstring_view key)
	{
		rjh::WriteKey(writer, key);
		writer.StartArray();
		for (const Rule &rule : vec)
		{
			rule.Serialize(writer);
		}
		writer.EndArray();
	}

	inline static void DeserializeRulesVec(const rjh::value_t& arr, std::vector<Rule>& vec, std::wstring_view key, void (*unknownKeyCallback)(std::wstring_view))
	{
		rjh::EnsureType(rj::Type::kArrayType, arr.GetType(), key);

		for (const auto &elem : arr.GetArray())
		{
			rjh::EnsureType(rj::Type::kObjectType, elem.GetType(), L"array element");
			Rule rule = Rule();
			rule.Deserialize(elem, unknownKeyCallback);
			vec.push_back(rule);
		}
	}

	static constexpr std::wstring_view RULES_KEY = L"rules";
};
