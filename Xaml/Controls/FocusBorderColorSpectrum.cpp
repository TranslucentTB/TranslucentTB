#include "pch.h"

#include "FocusBorderColorSpectrum.h"
#if __has_include("Controls/FocusBorderColorSpectrum.g.cpp")
#include "Controls/FocusBorderColorSpectrum.g.cpp"
#endif

namespace winrt::TranslucentTB::Xaml::Controls::implementation
{
	void FocusBorderColorSpectrum::OnApplyTemplate()
	{
		m_SizingGridSizeChangedRevoker.revoke();

		m_FocusBorder = GetTemplateChild(L"FocusBorder").try_as<wuxc::Border>();

		const auto sizingGrid = GetTemplateChild(L"SizingGrid").try_as<wux::FrameworkElement>();

		base_type::OnApplyTemplate();

		if (m_FocusBorder && sizingGrid)
		{
			UpdateFocusBorder(sizingGrid);
			m_SizingGridSizeChangedRevoker = sizingGrid.SizeChanged(winrt::auto_revoke, { get_weak(), &FocusBorderColorSpectrum::OnSizingGridSizeChanged });
		}
	}

	void FocusBorderColorSpectrum::OnSizingGridSizeChanged(const IInspectable &sender, const wux::SizeChangedEventArgs &)
	{
		if (const auto sizingGrid = sender.try_as<wux::FrameworkElement>())
		{
			UpdateFocusBorder(sizingGrid);
		}
	}

	void FocusBorderColorSpectrum::UpdateFocusBorder(const wux::FrameworkElement &sizingGrid)
	{
		m_FocusBorder.Width(sizingGrid.Width());
		m_FocusBorder.Height(sizingGrid.Height());
	}
}
