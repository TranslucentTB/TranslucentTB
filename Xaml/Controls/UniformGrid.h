#pragma once
#include "../dependencyproperty.h"
#include "../factory.h"
#include "winrt.hpp"

#include <experimental/generator>
#include <tuple>
#include <vector>

#include "Controls/UniformGrid.g.h"

namespace winrt::TranslucentTB::Xaml::Controls::implementation
{
	struct UniformGrid : UniformGridT<UniformGrid>
	{
	private:
		static void OnDependencyPropertyChanged(const IInspectable &ender, const wux::DependencyPropertyChangedEventArgs &args);

	public:
		wf::Size MeasureOverride(const wf::Size &availableSize);
		wf::Size ArrangeOverride(const wf::Size &finalSize);

		DECL_DEPENDENCY_PROPERTY_WITH_METADATA(int32_t, Columns,
			wux::PropertyMetadata(box_value(0), OnDependencyPropertyChanged));
		DECL_DEPENDENCY_PROPERTY_WITH_METADATA(int32_t, FirstColumn,
			wux::PropertyMetadata(box_value(0), OnDependencyPropertyChanged));
		DECL_DEPENDENCY_PROPERTY_WITH_METADATA(wuxc::Orientation, Orientation,
			wux::PropertyMetadata(box_value(wuxc::Orientation::Horizontal), OnDependencyPropertyChanged));
		DECL_DEPENDENCY_PROPERTY_WITH_METADATA(int32_t, Rows,
			wux::PropertyMetadata(box_value(0), OnDependencyPropertyChanged));

		static wux::DependencyProperty AutoLayoutProperty() noexcept
		{
			return m_AutoLayoutProperty;
		}

		static wf::IReference<bool> GetAutoLayout(const wux::FrameworkElement &element)
		{
			if (element)
			{
				return element.GetValue(m_AutoLayoutProperty).as<wf::IReference<bool>>();
			}
			else
			{
				return nullptr;
			}
		}

		static void SetAutoLayout(const wux::FrameworkElement &element, const wf::IReference<bool> &value)
		{
			if (element)
			{
				element.SetValue(m_AutoLayoutProperty, value);
			}
		}

	private:
		static wf::IReference<bool> GetAutoLayout(const wuxc::RowDefinition &element)
		{
			if (element)
			{
				return element.GetValue(m_AutoLayoutProperty).as<wf::IReference<bool>>();
			}
			else
			{
				return nullptr;
			}
		}

		static void SetAutoLayout(const wuxc::RowDefinition &element, const wf::IReference<bool> &value)
		{
			if (element)
			{
				element.SetValue(m_AutoLayoutProperty, value);
			}
		}

		static wf::IReference<bool> GetAutoLayout(const wuxc::ColumnDefinition&element)
		{
			if (element)
			{
				return element.GetValue(m_AutoLayoutProperty).as<wf::IReference<bool>>();
			}
			else
			{
				return nullptr;
			}
		}

		static void SetAutoLayout(const wuxc::ColumnDefinition &element, const wf::IReference<bool> &value)
		{
			if (element)
			{
				element.SetValue(m_AutoLayoutProperty, value);
			}
		}

		static std::tuple<int, int> GetDimensions(const std::vector<wux::FrameworkElement> &visible, int rows, int cols, int firstColumn);
		void SetupRowDefinitions(uint32_t rows);
		void SetupColumnDefinitions(uint32_t columns);

		bool GetSpot(int i, int j);
		void FillSpots(bool value, int row, int column, int width, int height);
		std::experimental::generator<std::tuple<int, int>> GetFreeSpots(int firstColumn, bool topDown);

		static wux::DependencyProperty m_AutoLayoutProperty;

		std::vector<bool> m_TakenSpots;
		int m_SpotsHeight = 0, m_SpotsWidth = 0;

		std::vector<wux::UIElement> m_Overflow;
	};
}

FACTORY(winrt::TranslucentTB::Xaml::Controls, UniformGrid);

