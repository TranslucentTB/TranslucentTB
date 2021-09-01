#pragma once
#include "factory.h"
#include "winrt.hpp"

#include <utility>
#include <vector>

#include "RelativeAncestor.g.h"

namespace winrt::TranslucentTB::Xaml::implementation
{
	struct RelativeAncestor
	{
		static wux::DependencyProperty AncestorProperty() noexcept
		{
			return m_AncestorProperty;
		}

		static wf::IInspectable GetAncestor(const wux::FrameworkElement &obj)
		{
			if (obj)
			{
				return obj.GetValue(m_AncestorProperty);
			}
			else
			{
				return nullptr;
			}
		}

		static void SetAncestor(const wux::FrameworkElement &obj, const wf::IInspectable &value)
		{
			if (obj)
			{
				obj.SetValue(m_AncestorProperty, value);
			}
		}

		static wux::DependencyProperty AncestorTypeProperty() noexcept
		{
			return m_AncestorTypeProperty;
		}

		static wux::Interop::TypeName GetAncestorType(const wux::FrameworkElement &obj)
		{
			return obj.GetValue(m_AncestorTypeProperty).as<wux::Interop::TypeName>();
		}

		static void SetAncestorType(const wux::FrameworkElement &obj, const wux::Interop::TypeName &value)
		{
			if (obj)
			{
				obj.SetValue(m_AncestorTypeProperty, box_value(value));
			}
		}

	private:
		static void OnAncestorTypeChanged(const wux::DependencyObject &d, const wux::DependencyPropertyChangedEventArgs &e);
		static void OnFrameworkElementLoaded(const wf::IInspectable &sender, const wux::RoutedEventArgs &args);
		static void OnFrameworkElementUnloaded(const wf::IInspectable &sender, const wux::RoutedEventArgs &args);
		static wux::DependencyObject FindAscendant(const wux::DependencyObject &element, const wux::Interop::TypeName &type);
		static void RemoveHandlers(const wux::FrameworkElement &element) noexcept;

		static wux::DependencyProperty m_AncestorProperty;
		static wux::DependencyProperty m_AncestorTypeProperty;

		static thread_local std::vector<std::pair<weak_ref<wux::FrameworkElement>, event_token>> s_TokenList;
	};
}

FACTORY(winrt::TranslucentTB::Xaml, RelativeAncestor);
