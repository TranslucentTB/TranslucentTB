#pragma once
#include "../../factory.h"
#include "../../property.h"

#include "TaskbarAppearance.h"
#include "Models/Primitives/OptionalTaskbarAppearance.g.h"

namespace winrt::TranslucentTB::Xaml::Models::Primitives::implementation
{
	struct OptionalTaskbarAppearance : OptionalTaskbarAppearanceT<OptionalTaskbarAppearance, TaskbarAppearance>
	{
		OptionalTaskbarAppearance() noexcept = default;
		OptionalTaskbarAppearance(bool enabled, AccentState accent, Windows::UI::Color color, bool showPeek, bool showLine) noexcept :
			base_type(accent, color, showPeek, showLine),
			m_Enabled(enabled)
		{ }

		DECL_PROPERTY_FUNCS(bool, Enabled, m_Enabled);

	private:
		bool m_Enabled = false;
	};
}

FACTORY(winrt::TranslucentTB::Xaml::Models::Primitives, OptionalTaskbarAppearance);
