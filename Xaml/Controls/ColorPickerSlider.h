#pragma once
#include "../dependencyproperty.h"
#include "../factory.h"
#include "winrt.hpp"

#include "Controls/ColorPickerSlider.g.h"

namespace winrt::TranslucentTB::Xaml::Controls::implementation
{
	// Explicitly use ColorPickerSlider_base because the XAML compiler generates an empty .xaml.g.h
	struct ColorPickerSlider : ColorPickerSlider_base<ColorPickerSlider>
	{
	private:
		// make DECL_DEPENDENCY_PROPERTY_WITH_METADATA below work
		static void OnDependencyPropertyChanged(const IInspectable &sender, const wux::DependencyPropertyChangedEventArgs &args);

	public:
		ColorPickerSlider();
		~ColorPickerSlider();

		void UpdateColors();

		// Colors::White
		DECL_DEPENDENCY_PROPERTY_WITH_METADATA(Windows::UI::Color, Color,
			wux::PropertyMetadata(box_value(Windows::UI::Color { 0xFF, 0xFF, 0xFF, 0xFF }), OnDependencyPropertyChanged));

		DECL_DEPENDENCY_PROPERTY_WITH_METADATA(txmp::ColorChannel, ColorChannel,
			wux::PropertyMetadata(box_value(txmp::ColorChannel::Channel1), OnDependencyPropertyChanged));

		DECL_DEPENDENCY_PROPERTY_WITH_METADATA(txmp::ColorRepresentation, ColorRepresentation,
			wux::PropertyMetadata(box_value(txmp::ColorRepresentation::Rgba), OnDependencyPropertyChanged));

		// Colors::White but in HSV
		DECL_DEPENDENCY_PROPERTY_WITH_METADATA(txmp::HsvColor, HsvColor,
			wux::PropertyMetadata(box_value(txmp::HsvColor { 0.0, 0.0, 1.0, 1.0 }), OnDependencyPropertyChanged));

		DECL_DEPENDENCY_PROPERTY_WITH_METADATA(bool, IsAlphaMaxForced,
			wux::PropertyMetadata(box_value(true), OnDependencyPropertyChanged));

		DECL_DEPENDENCY_PROPERTY_WITH_METADATA(bool, IsAutoUpdatingEnabled,
			wux::PropertyMetadata(box_value(true), OnDependencyPropertyChanged));

		DECL_DEPENDENCY_PROPERTY_WITH_METADATA(bool, IsSaturationValueMaxForced,
			wux::PropertyMetadata(box_value(true), OnDependencyPropertyChanged));

		// Gray-ish?
		// Doesn't really matter, overriden in the template for SystemListLowColor
		DECL_DEPENDENCY_PROPERTY_WITH_METADATA(Windows::UI::Color, CheckerBackgroundColor,
			wux::PropertyMetadata(box_value(Windows::UI::Color { 0x19, 0x80, 0x80, 0x80 }), OnDependencyPropertyChanged));

		void OnApplyTemplate();
		wf::Size MeasureOverride(const wf::Size &availableSize);

	private:
		wf::Size oldSize = {};
		wf::Size measuredSize = {};
		wf::Size cachedSize = {};

		event_token m_OrientationPropertyChangedToken;
		static void OnOrientationPropertyChanged(const wux::DependencyObject &sender, const wux::DependencyProperty &prop);

		wux::Shapes::Rectangle m_HorizontalTrackRect = nullptr;
		wux::Shapes::Rectangle m_VerticalTrackRect = nullptr;

		event_token m_LoadedToken, m_UnloadedToken;
		void OnLoaded(const IInspectable &sender, const wux::RoutedEventArgs &args);
		void OnUnloaded(const IInspectable &sender, const wux::RoutedEventArgs &args);

		// revoker is OK here because we don't need to hold a strong ref
		wux::XamlRoot::Changed_revoker m_XamlRootChangedRevoker;
		void OnXamlRootChanged(const wux::XamlRoot &sender, const wux::XamlRootChangedEventArgs &args);
		wf::TypedEventHandler<wux::XamlRoot, wux::XamlRootChangedEventArgs> m_XamlRootChangedHandler = { get_weak(), &ColorPickerSlider::OnXamlRootChanged };

		void UpdateVisualState();
		void UpdateBackground(txmp::HsvColor color);
		fire_and_forget RenderBackground(weak_ref<wux::Shapes::Rectangle> rect_weak, txmp::HsvColor color, wuxc::Orientation orientation, int32_t width, int32_t height, double renderScale);
	};
}

FACTORY(winrt::TranslucentTB::Xaml::Controls, ColorPickerSlider);
