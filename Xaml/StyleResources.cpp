#include "pch.h"
#include <algorithm>

#include "StyleResources.h"
#if __has_include("StyleResources.g.cpp")
#include "StyleResources.g.cpp"
#endif

namespace winrt::TranslucentTB::Xaml::implementation
{
	wux::DependencyProperty StyleResources::m_ResourcesProperty =
		wux::DependencyProperty::RegisterAttached(
			L"Resources",
			xaml_typename<wux::ResourceDictionary>(),
			xaml_typename<TranslucentTB::Xaml::StyleResources>(),
			wux::PropertyMetadata(nullptr, OnResourcesChanged));

	void StyleResources::OnResourcesChanged(const wux::DependencyObject &d, const wux::DependencyPropertyChangedEventArgs &e)
	{
		if (const auto source = d.try_as<wux::FrameworkElement>())
		{
			wfc::IVector<wux::ResourceDictionary> mergedDicts(nullptr);
			if (const auto resources = source.Resources())
			{
				mergedDicts = resources.MergedDictionaries();
			}

			if (mergedDicts)
			{
				const auto begin = mergedDicts.begin();
				const auto end = mergedDicts.end();
				const auto it = std::ranges::find_if(begin, end, [](const wux::ResourceDictionary &resDict)
				{
					return resDict.try_as<IStyleResourceDictionary>() != nullptr;
				});

				if (it != end)
				{
					mergedDicts.RemoveAt(static_cast<uint32_t>(it - begin));
				}

				if (const auto newVal = e.NewValue().try_as<wux::ResourceDictionary>())
				{
					const auto clonedRes = winrt::make<StyleResourceDictionary>();
					CloneResourceDictionary(newVal, clonedRes);
					mergedDicts.Append(clonedRes);
				}
			}
		}
	}

	wux::ResourceDictionary StyleResources::CloneResourceDictionary(const wux::ResourceDictionary &resource, const wux::ResourceDictionary &destination)
	{
		if (!resource)
		{
			return nullptr;
		}

		if (const auto source = resource.Source())
		{
			destination.Source(source);
		}
		else
		{
			if (const auto themeDicts = resource.ThemeDictionaries())
			{
				for (const auto theme : themeDicts)
				{
					if (const auto themeResource = theme.Value().try_as<wux::ResourceDictionary>())
					{
						wux::ResourceDictionary themeDict;
						CloneResourceDictionary(themeResource, themeDict);
						destination.ThemeDictionaries().Insert(theme.Key(), themeDict);
					}
					else
					{
						destination.ThemeDictionaries().Insert(theme.Key(), theme.Value());
					}
				}
			}

			if (const auto mergeDicts = resource.MergedDictionaries())
			{
				for (const auto mergedResource : mergeDicts)
				{
					wux::ResourceDictionary mergedDict;
					CloneResourceDictionary(mergedResource, mergedDict);
					destination.MergedDictionaries().Append(mergedDict);
				}
			}

			for (const auto item : resource)
			{
				destination.Insert(item.Key(), item.Value());
			}
		}

		return destination;
	}
}
