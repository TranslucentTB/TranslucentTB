#pragma once
#include "winrt.hpp"
#include <winrt/Windows.UI.Xaml.Interop.h>
#include <winrt/Windows.UI.Xaml.Markup.h>

namespace winrt::TranslucentTB::Xaml::implementation {
	template<typename D, typename... I>
	struct App_baseWithProvider : public App_base<D, wux::Markup::IXamlMetadataProvider> {
		using IXamlType = wux::Markup::IXamlType;

		IXamlType GetXamlType(const wux::Interop::TypeName &type)
		{
			return AppProvider()->GetXamlType(type);
		}

		IXamlType GetXamlType(const hstring &fullName)
		{
			return AppProvider()->GetXamlType(fullName);
		}

		com_array<wux::Markup::XmlnsDefinition> GetXmlnsDefinitions()
		{
			return AppProvider()->GetXmlnsDefinitions();
		}

	private:
		bool _contentLoaded { false };
		com_ptr<XamlMetaDataProvider> _appProvider;
		com_ptr<XamlMetaDataProvider> AppProvider()
		{
			if (!_appProvider)
			{
				_appProvider = make_self<XamlMetaDataProvider>();
			}
			return _appProvider;
		}
	};

	template<typename D, typename... I>
	using AppT2 = App_baseWithProvider<D, I...>;
}
