#include "pch.h"

#include "FocusBorderColorSpectrum.h"
#include <cmath>
#if __has_include("Controls/FocusBorderColorSpectrum.g.cpp")
#include "Controls/FocusBorderColorSpectrum.g.cpp"
#endif

namespace winrt::TranslucentTB::Xaml::Controls::implementation
{
	FocusBorderColorSpectrum::FocusBorderColorSpectrum()
	{
		UseLayoutRounding(false);
	}

	wf::Size FocusBorderColorSpectrum::ArrangeOverride(const wf::Size &finalSize)
	{
		// WinUI 2 rounds the color-map dimensions, but compares rounded pointer
		// coordinates with the original fractional dimensions. At 258.4 DIPs,
		// coordinate 258 is accepted although the final valid pixel is 257.
		// Arrange the spectrum in whole DIPs to keep those bounds identical.
		// Physical-pixel layout rounding must also stay disabled: at fractional
		// display scales it would introduce fractional DIPs again.
		if (m_LayoutRoot)
		{
			m_LayoutRoot.Width(std::floor(finalSize.Width));
			m_LayoutRoot.Height(std::floor(finalSize.Height));
		}
		return base_type::ArrangeOverride(finalSize);
	}

	void FocusBorderColorSpectrum::OnApplyTemplate()
	{
		m_SizingGridSizeChangedRevoker.revoke();

		m_FocusBorder = GetTemplateChild(L"FocusBorder").try_as<wuxc::Border>();

		const auto sizingGrid = GetTemplateChild(L"SizingGrid").try_as<wux::FrameworkElement>();
		m_LayoutRoot = GetTemplateChild(L"LayoutRoot").try_as<wux::FrameworkElement>();
		if (m_LayoutRoot)
		{
			m_LayoutRoot.UseLayoutRounding(false);
		}

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
