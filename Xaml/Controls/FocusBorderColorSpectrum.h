#pragma once
#include "../factory.h"
#include "winrt.hpp"

#include "Controls/FocusBorderColorSpectrum.g.h"

namespace winrt::TranslucentTB::Xaml::Controls::implementation
{
    struct FocusBorderColorSpectrum : FocusBorderColorSpectrum_base<FocusBorderColorSpectrum>
    {
        FocusBorderColorSpectrum();

		void OnApplyTemplate();
		wf::Size ArrangeOverride(const wf::Size &finalSize);

	private:
		wux::FrameworkElement::SizeChanged_revoker m_SizingGridSizeChangedRevoker;
		wuxc::Border m_FocusBorder;
		wux::FrameworkElement m_LayoutRoot { nullptr };

		void OnSizingGridSizeChanged(const IInspectable &sender, const wux::SizeChangedEventArgs &args);
		void UpdateFocusBorder(const wux::FrameworkElement &sizingGrid);
    };
}

FACTORY(winrt::TranslucentTB::Xaml::Controls, FocusBorderColorSpectrum);
