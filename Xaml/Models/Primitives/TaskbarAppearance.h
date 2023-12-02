#pragma once
#include "../../factory.h"
#include "../../property.h"

#include "Models/Primitives/TaskbarAppearance.g.h"

namespace winrt::TranslucentTB::Xaml::Models::Primitives::implementation
{
	struct TaskbarAppearance : TaskbarAppearanceT<TaskbarAppearance>
	{
		TaskbarAppearance() noexcept = default;
		TaskbarAppearance(AccentState accent, Windows::UI::Color color, bool showPeek, bool showLine, uint32_t blurRadius = 3) noexcept :
			m_Accent(accent),
			m_Color(color),
			m_Radius(blurRadius),
			m_ShowPeek(showPeek),
			m_ShowLine(showLine)
		{ }

		DECL_PROPERTY_FUNCS(AccentState, Accent, m_Accent);
		DECL_PROPERTY_FUNCS(Windows::UI::Color, Color, m_Color);
		DECL_PROPERTY_FUNCS(uint32_t, BlurRadius, m_Radius);
		DECL_PROPERTY_FUNCS(bool, ShowPeek, m_ShowPeek);
		DECL_PROPERTY_FUNCS(bool, ShowLine, m_ShowLine);

	private:
		AccentState m_Accent = AccentState::Normal;
		Windows::UI::Color m_Color = { 0, 0, 0, 0 };
		uint32_t m_Radius = 3.0f;
		bool m_ShowPeek = true;
		bool m_ShowLine = true;
	};
}

FACTORY(winrt::TranslucentTB::Xaml::Models::Primitives, TaskbarAppearance);
