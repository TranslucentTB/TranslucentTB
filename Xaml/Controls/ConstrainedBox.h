#pragma once
#include "../dependencyproperty.h"
#include "../factory.h"
#include "winrt.hpp"

#include "Controls/ConstrainedBox.g.h"

namespace winrt::TranslucentTB::Xaml::Controls::implementation
{
    struct ConstrainedBox : ConstrainedBoxT<ConstrainedBox>
    {
	private:
		// make DECL_DEPENDENCY_PROPERTY_WITH_METADATA below work
		static void OnDependencyPropertyChanged(const IInspectable &sender, const wux::DependencyPropertyChangedEventArgs &args);

	public:
        ConstrainedBox() = default;

		wf::Size MeasureOverride(wf::Size availableSize);
		wf::Size ArrangeOverride(wf::Size finalSize);

		DECL_DEPENDENCY_PROPERTY_WITH_METADATA(int32_t, MultipleX,
			wux::PropertyMetadata(box_value(1), OnDependencyPropertyChanged));

		DECL_DEPENDENCY_PROPERTY_WITH_METADATA(int32_t, MultipleY,
			wux::PropertyMetadata(box_value(1), OnDependencyPropertyChanged));

	private:
		static bool IsPositiveRealNumber(double value) noexcept;
		static void ApplyMultiple(int32_t multiple, float &value) noexcept;
		void CalculateConstrainedSize(wf::Size &availableSize);

		// Value used to determine when we re-calculate in the arrange step or re-use a previous calculation. Within roughly a pixel seems like a good value?
		static constexpr double CALCULATION_TOLERANCE = 1.5;

		wf::Size m_OriginalSize = { };
		wf::Size m_LastMeasuredSize = { };
    };
}

FACTORY(winrt::TranslucentTB::Xaml::Controls, ConstrainedBox);
