#include "pch.h"

#include "RelativeAncestor.h"
#if __has_include("RelativeAncestor.g.cpp")
#include "RelativeAncestor.g.cpp"
#endif

namespace winrt::TranslucentTB::Xaml::implementation
{
	thread_local std::vector<std::pair<weak_ref<wux::FrameworkElement>, event_token>> RelativeAncestor::s_TokenList;

	wux::DependencyProperty RelativeAncestor::m_AncestorProperty =
		wux::DependencyProperty::RegisterAttached(
			L"Ancestor",
			xaml_typename<wf::IInspectable>(),
			xaml_typename<TranslucentTB::Xaml::RelativeAncestor>(),
			nullptr);

	wux::DependencyProperty RelativeAncestor::m_AncestorTypeProperty =
		wux::DependencyProperty::RegisterAttached(
			L"AncestorType",
			xaml_typename<wux::Interop::TypeName>(),
			xaml_typename<TranslucentTB::Xaml::RelativeAncestor>(),
			wux::PropertyMetadata(nullptr, OnAncestorTypeChanged));

	void RelativeAncestor::OnAncestorTypeChanged(const wux::DependencyObject &d, const wux::DependencyPropertyChangedEventArgs &e)
	{
		if (const auto fe = d.try_as<wux::FrameworkElement>())
		{
			RemoveHandlers(fe);

			if (e.NewValue())
			{
				event_token unloadedToken = fe.Unloaded(OnFrameworkElementUnloaded);
				try
				{
					s_TokenList.push_back({ fe, unloadedToken });
				}
				catch (...)
				{
					// just in case something happens
					fe.Unloaded(unloadedToken);
					throw;
				}

				event_token loadedToken = fe.Loaded(OnFrameworkElementLoaded);
				try
				{
					s_TokenList.push_back({ fe, loadedToken });
				}
				catch (...)
				{
					// just in case something happens
					fe.Loaded(loadedToken);
					throw;
				}

				if (fe.Parent())
				{
					OnFrameworkElementLoaded(fe, nullptr);
				}
			}
		}
	}

	void RelativeAncestor::OnFrameworkElementLoaded(const wf::IInspectable &sender, const wux::RoutedEventArgs &)
	{
		if (const auto fe = sender.try_as<wux::FrameworkElement>())
		{
			SetAncestor(fe, FindAscendant(fe, GetAncestorType(fe)));
		}
	}

	void RelativeAncestor::OnFrameworkElementUnloaded(const wf::IInspectable &sender, const wux::RoutedEventArgs &)
	{
		if (const auto fe = sender.try_as<wux::FrameworkElement>())
		{
			// avoid an XAML leak
			SetAncestor(fe, nullptr);
		}
	}

	wux::DependencyObject RelativeAncestor::FindAscendant(const wux::DependencyObject &element, const wux::Interop::TypeName &type)
	{
		const auto parent = wux::Media::VisualTreeHelper::GetParent(element);

		// IsAssignableFrom doesn't exist here but then the WCT also just checks the concrete type, so whatever.
		if (!parent || get_class_name(parent) == type.Name)
		{
			return parent;
		}
		else
		{
			// haha recursive function go brrr
			return FindAscendant(parent, type);
		}
	}

	void RelativeAncestor::RemoveHandlers(const wux::FrameworkElement &element) noexcept
	{
		for (auto it = s_TokenList.begin(); it != s_TokenList.end();)
		{
			if (const auto fe = it->first.get())
			{
				if (fe == element)
				{
					fe.Loaded(it->second);
				}

				++it;
			}
			else
			{
				it = s_TokenList.erase(it);
			}
		}
	}
}
