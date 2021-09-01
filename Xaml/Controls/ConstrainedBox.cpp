#include "pch.h"
#include <cmath>

#include "ConstrainedBox.h"
#if __has_include("Controls/ConstrainedBox.g.cpp")
#include "Controls/ConstrainedBox.g.cpp"
#endif

namespace winrt::TranslucentTB::Xaml::Controls::implementation
{
	void ConstrainedBox::OnDependencyPropertyChanged(const IInspectable &sender, const wux::DependencyPropertyChangedEventArgs &)
	{
		if (const auto that = sender.try_as<ConstrainedBox>())
		{
			that->InvalidateMeasure();
		}
	}

	wf::Size ConstrainedBox::MeasureOverride(wf::Size availableSize)
	{
		m_OriginalSize = availableSize;

		CalculateConstrainedSize(availableSize);

		m_LastMeasuredSize = availableSize;

		// Call base_type::MeasureOverride so any child elements know what room there is to work with.
		// Don't return this though. An image that hasn't loaded yet for example will request very little space.
		base_type::MeasureOverride(m_LastMeasuredSize);
		return m_LastMeasuredSize;
	}

	wf::Size ConstrainedBox::ArrangeOverride(wf::Size finalSize)
	{
		// Even though we requested in measure to be a specific size, that doesn't mean our parent
		// panel respected that request. Grid for instance can by default Stretch and if you don't
		// set Horizontal/VerticalAlignment on the control it won't constrain as we expect.
		// We could also be in a StackPanel/ScrollViewer where it wants to provide as much space as possible.
		// However, if we always re-calculate even if we are provided the proper finalSize, this can trigger
		// multiple arrange passes and cause a rounding error in layout. Therefore, we only want to
		// re-calculate if we think we will have a significant impact.
		if (std::abs(finalSize.Width - m_LastMeasuredSize.Width) > CALCULATION_TOLERANCE ||
			std::abs(finalSize.Height - m_LastMeasuredSize.Height) > CALCULATION_TOLERANCE)
		{
			// Check if we can re-use our measure calculation if we're given effectively
			// the same size as we had in the measure step.
			if (std::abs(finalSize.Width - m_OriginalSize.Width) <= CALCULATION_TOLERANCE &&
				std::abs(finalSize.Height - m_OriginalSize.Height) <= CALCULATION_TOLERANCE)
			{
				finalSize = m_LastMeasuredSize;
			}
			else
			{
				CalculateConstrainedSize(finalSize);

				// Copy again so if Arrange is re-triggered we won't re-re-calculate.
				m_LastMeasuredSize = finalSize;
			}
		}

		return base_type::ArrangeOverride(finalSize);
	}

	bool ConstrainedBox::IsPositiveRealNumber(double value) noexcept
	{
		return std::isfinite(value) && value > 0.0;
	}

	void ConstrainedBox::ApplyMultiple(int32_t multiple, float &value) noexcept
	{
		if (multiple == 0)
		{
			value = 0.0f;
		}
		else if (multiple > 1)
		{
			value = std::floor(value / multiple) * multiple;
		}
	}

	void ConstrainedBox::CalculateConstrainedSize(wf::Size &availableSize)
	{
		// We check for Infinity, in the case we have no constraint from parent
		// we'll request the child's measurements first, so we can use that as
		// a starting point to constrain it's dimensions based on the criteria
		// set in our properties.
		bool hasWidth = IsPositiveRealNumber(availableSize.Width);
		bool hasHeight = IsPositiveRealNumber(availableSize.Height);

		if (!hasWidth && !hasHeight)
		{
			// We have infinite space, like a ScrollViewer with both scrolling directions
			// Ask child how big they want to be first.
			availableSize = base_type::MeasureOverride(availableSize);

			hasWidth = IsPositiveRealNumber(availableSize.Width);
			hasHeight = IsPositiveRealNumber(availableSize.Height);

			if (!hasWidth && !hasHeight)
			{
				// At this point we have no way to determine a constraint, the Panel won't do anything
				// This should be rare? We don't really have a way to provide a warning here.
				return;
			}
		}

		// Apply Multiples
		// ---------------
		// These floor the Width/Height values to the nearest multiple of the property (if set).
		// For instance you may have a responsive 4x4 repeated checkerboard pattern for transparency and
		// want to snap to the nearest interval of 4 so the checkerboard is consistency across the layout.
		if (hasWidth)
		{
			ApplyMultiple(MultipleX(), availableSize.Width);
		}

		if (hasHeight)
		{
			ApplyMultiple(MultipleY(), availableSize.Height);
		}
	}
}
