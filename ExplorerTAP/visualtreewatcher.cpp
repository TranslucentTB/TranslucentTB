#include "visualtreewatcher.hpp"
#include "undefgetcurrenttime.h"
#include <winrt/Windows.UI.Xaml.Hosting.h>
#include <winrt/Windows.UI.Xaml.Media.h>
#include "redefgetcurrenttime.h"

HRESULT VisualTreeWatcher::OnVisualTreeChange(ParentChildRelation, VisualElement element, VisualMutationType mutationType) try
{
	const std::wstring_view type { element.Type, SysStringLen(element.Type) };
	if (type == winrt::name_of<wuxh::DesktopWindowXamlSource>())
	{
		switch (mutationType)
		{
		case Add:
			if (!m_FoundSources.insert(element.Handle).second)
			{
				if (const auto background = FindControl(FromHandle<wuxh::DesktopWindowXamlSource>(element.Handle).Content().try_as<wux::FrameworkElement>(), L"BackgroundFill"))
				{
					background.Visibility(wux::Visibility::Collapsed);
				}
			}
			break;

		case Remove:
			m_FoundSources.erase(element.Handle);
			break;
		}
	}

	return S_OK;
}
catch (...)
{
	return winrt::to_hresult();
}

HRESULT VisualTreeWatcher::OnElementStateChanged(InstanceHandle, VisualElementState, LPCWSTR) noexcept
{
	return S_OK;
}

void VisualTreeWatcher::SetXamlDiagnostics(winrt::com_ptr<IXamlDiagnostics> diagnostics) noexcept
{
	m_XamlDiagnostics = std::move(diagnostics);
	m_FoundSources.clear();
}

wux::FrameworkElement VisualTreeWatcher::FindControl(const wux::FrameworkElement &parent, std::wstring_view name)
{
	if (!parent)
	{
		return nullptr;
	}

	if (parent.Name() == name)
	{
		return parent;
	}

	const int32_t count = wux::Media::VisualTreeHelper::GetChildrenCount(parent);
	for (int32_t i = 0; i < count; i++)
	{
		if (const auto element = FindControl(wux::Media::VisualTreeHelper::GetChild(parent, i).try_as<wux::FrameworkElement>(), name))
		{
			return element;
		}
	}

	return nullptr;
}
