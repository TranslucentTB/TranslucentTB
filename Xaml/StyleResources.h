#pragma once
#include "factory.h"

#include "StyleResources.g.h"

namespace winrt::TranslucentTB::Xaml::implementation
{
	struct StyleResources
	{
		static wux::DependencyProperty ResourcesProperty() noexcept
		{
			return m_ResourcesProperty;
		}

		static wux::ResourceDictionary GetResources(const wux::DependencyObject &obj)
		{
			return obj.GetValue(m_ResourcesProperty).as<wux::ResourceDictionary>();
		}

		static void SetResources(const wux::DependencyObject &obj, const wux::ResourceDictionary &value)
		{
			obj.SetValue(m_ResourcesProperty, value);
		}

	private:
		static void OnResourcesChanged(const wux::DependencyObject &d, const wux::DependencyPropertyChangedEventArgs &e);
		static wux::ResourceDictionary CloneResourceDictionary(const wux::ResourceDictionary &resource, const wux::ResourceDictionary &destination);

		static wux::DependencyProperty m_ResourcesProperty;
	};
}

FACTORY(winrt::TranslucentTB::Xaml, StyleResources);
