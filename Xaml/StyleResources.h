#pragma once
#include "factory.h"
#include "winrt.hpp"

#include "StyleResources.g.h"

namespace winrt::TranslucentTB::Xaml::implementation
{
	// ResourceDictionary with marker interface to be able to find out the MergedDictionary we inserted
	MIDL_INTERFACE("A240AC45-8961-4B2F-9259-7F9C8740BFFC") IStyleResourceDictionary : IUnknown
	{
	};

	struct StyleResourceDictionary : wux::ResourceDictionaryT<StyleResourceDictionary, IStyleResourceDictionary>
	{
	};

	struct StyleResources
	{
		static wux::DependencyProperty ResourcesProperty() noexcept
		{
			return m_ResourcesProperty;
		}

		static wux::ResourceDictionary GetResources(const wux::DependencyObject &obj)
		{
			if (obj)
			{
				return obj.GetValue(m_ResourcesProperty).as<wux::ResourceDictionary>();
			}
			else
			{
				return nullptr;
			}
		}

		static void SetResources(const wux::DependencyObject &obj, const wux::ResourceDictionary &value)
		{
			if (obj)
			{
				obj.SetValue(m_ResourcesProperty, value);
			}
		}

	private:
		static void OnResourcesChanged(const wux::DependencyObject &d, const wux::DependencyPropertyChangedEventArgs &e);
		static wux::ResourceDictionary CloneResourceDictionary(const wux::ResourceDictionary &resource, const wux::ResourceDictionary &destination);

		static wux::DependencyProperty m_ResourcesProperty;
	};
}

FACTORY(winrt::TranslucentTB::Xaml, StyleResources);
