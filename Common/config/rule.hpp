#include "taskbarappearance.hpp"

struct Rule : TaskbarAppearance
{
	std::wstring m_Class;
	std::wstring m_Title;
	std::wstring m_ProcessName;

	constexpr Rule() noexcept = default;
	constexpr Rule(std::wstring m_class, std::wstring m_title, std::wstring m_processName, ACCENT_STATE accent, Util::Color color, bool showPeek) noexcept :
		TaskbarAppearance(accent, color, showPeek),
		m_Class(m_class),
		m_Title(m_title),
		m_ProcessName(m_processName)
	{
	}

	template <typename Writer>
	inline void Serialize(Writer &writer) const
	{
		writer.StartObject();
		rjh::Serialize(writer, m_Class, CLASS_KEY);
		rjh::Serialize(writer, m_Title, TITLE_KEY);
		rjh::Serialize(writer, m_ProcessName, PROCESS_NAME_KEY);
		TaskbarAppearance::Serialize(writer);
		writer.EndObject();
	}

	void Deserialize(const rjh::value_t &obj, void (*unknownKeyCallback)(std::wstring_view))
	{
		rjh::EnsureType(rj::Type::kObjectType, obj.GetType(), L"root node");

		for (auto it = obj.MemberBegin(); it != obj.MemberEnd(); ++it)
		{
			rjh::EnsureType(rj::Type::kStringType, it->name.GetType(), L"member name");

			const auto key = rjh::ValueToStringView(it->name);
			if (key == CLASS_KEY)
			{
				m_Class =	rjh::ValueToStringView(it->value);
			}
			else if (key == TITLE_KEY)
			{
				m_Title = rjh::ValueToStringView(it->value);
			}
			else if (key == PROCESS_NAME_KEY)
			{
				m_ProcessName = rjh::ValueToStringView(it->value);
			}
			else
			{
				InnerDeserialize(key, it->value, unknownKeyCallback);
			}
		}
	}

	static constexpr std::wstring_view CLASS_KEY = L"window_class";
	static constexpr std::wstring_view TITLE_KEY = L"window_title";
	static constexpr std::wstring_view PROCESS_NAME_KEY = L"process_name";
};
