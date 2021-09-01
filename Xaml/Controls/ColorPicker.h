#pragma once
#include "../dependencyproperty.h"
#include "../factory.h"
#include "winrt.hpp"

#include "Controls/ColorPicker.g.h"

namespace winrt::TranslucentTB::Xaml::Controls::implementation
{
	// Explicitly use ColorPicker_base because the XAML compiler generates an empty .xaml.g.h
	struct ColorPicker : ColorPicker_base<ColorPicker>
	{
	private:
		// make DECL_DEPENDENCY_PROPERTY_WITH_METADATA below work
		static void OnDependencyPropertyChanged(const IInspectable &sender, const wux::DependencyPropertyChangedEventArgs &args);

		// This one manually implemented to not expose the setter
		DECL_DEPENDENCY_PROPERTY_FIELD(wfc::IObservableVector<Windows::UI::Color>, CustomPaletteColors, nullptr);

	public:
		ColorPicker();
		~ColorPicker();

		void OnApplyTemplate();

		static wux::DependencyProperty CustomPaletteColorsProperty() noexcept
		{
			return DEPENDENCY_PROPERTY_FIELD(CustomPaletteColors);
		}

		wfc::IObservableVector<Windows::UI::Color> CustomPaletteColors()
		{
			return GetValue(DEPENDENCY_PROPERTY_FIELD(CustomPaletteColors)).as<wfc::IObservableVector<Windows::UI::Color>>();
		}

		DECL_DEPENDENCY_PROPERTY_WITH_DEFAULT(int32_t, CustomPaletteColumnCount, box_value(4));

		DECL_DEPENDENCY_PROPERTY_WITH_METADATA(txmp::IColorPalette, CustomPalette,
			wux::PropertyMetadata(Models::FluentColorPalette(), OnDependencyPropertyChanged));

		DECL_DEPENDENCY_PROPERTY_WITH_METADATA(bool, IsColorPaletteVisible,
			wux::PropertyMetadata(box_value(true), OnDependencyPropertyChanged));

		// Gray-ish?
		// Doesn't really matter, overriden in the template for SystemListLowColor
		DECL_DEPENDENCY_PROPERTY_WITH_METADATA(Windows::UI::Color, CheckerBackgroundColor,
			wux::PropertyMetadata(box_value(Windows::UI::Color { 0x19, 0x80, 0x80, 0x80 }), OnDependencyPropertyChanged));

		DECL_DEPENDENCY_PROPERTY(wux::Media::Brush, HeaderBackground);
		DECL_DEPENDENCY_PROPERTY_WITH_DEFAULT(wux::CornerRadius, HeaderCornerRadius, box_value(wux::CornerRadius { }));

		static wux::DependencyProperty InvertedCheckerboardProperty() noexcept
		{
			return m_InvertedCheckerboardProperty;
		}

		static bool GetInvertedCheckerboard(const wuxc::Border &obj)
		{
			return obj.GetValue(m_InvertedCheckerboardProperty).as<bool>();
		}

		static void SetInvertedCheckerboard(const wuxc::Border &obj, bool value)
		{
			if (obj)
			{
				obj.SetValue(m_InvertedCheckerboardProperty, box_value(value));
			}
		}

	private:
		Windows::System::DispatcherQueueTimer m_DispatcherQueueTimer = Windows::System::DispatcherQueue::GetForCurrentThread().CreateTimer();
		Windows::Globalization::NumberFormatting::DecimalFormatter m_DecimalFormatter;
		std::optional<txmp::HsvColor> m_SavedHsvColor;
		std::optional<Windows::UI::Color> m_SavedHsvColorRgbEquivalent, m_UpdatedRgbColor;
		bool m_UpdateFromSpectrum = false;

		// template parts and event handlers
#define TEMPLATE_PART_1_EVENT(TYPE, NAME, EVENT) \
		TYPE m_ ## NAME = nullptr; \
		event_token m_ ## NAME ## EVENT ## Token;

#define TEMPLATE_PART_2_EVENTS(TYPE, NAME, EVENT1, EVENT2) \
		TYPE m_ ## NAME = nullptr; \
		event_token m_ ## NAME ## EVENT1 ## Token, m_ ## NAME ## EVENT2 ## Token;

		TEMPLATE_PART_2_EVENTS(Controls::ColorPickerSlider, Channel1Slider, ValueChanged, Loaded);
		TEMPLATE_PART_2_EVENTS(Controls::ColorPickerSlider, Channel2Slider, ValueChanged, Loaded);
		TEMPLATE_PART_2_EVENTS(Controls::ColorPickerSlider, Channel3Slider, ValueChanged, Loaded);
		TEMPLATE_PART_2_EVENTS(Controls::ColorPickerSlider, AlphaChannelSlider, ValueChanged, Loaded);
		TEMPLATE_PART_2_EVENTS(Controls::ColorPickerSlider, ColorSpectrumAlphaSlider, ValueChanged, Loaded);
		TEMPLATE_PART_2_EVENTS(Controls::ColorPickerSlider, ColorSpectrumThirdDimensionSlider, ValueChanged, Loaded);

		TEMPLATE_PART_2_EVENTS(wuxc::TextBox, Channel1TextBox, KeyDown, LostFocus);
		TEMPLATE_PART_2_EVENTS(wuxc::TextBox, Channel2TextBox, KeyDown, LostFocus);
		TEMPLATE_PART_2_EVENTS(wuxc::TextBox, Channel3TextBox, KeyDown, LostFocus);
		TEMPLATE_PART_2_EVENTS(wuxc::TextBox, AlphaChannelTextBox, KeyDown, LostFocus);
		TEMPLATE_PART_2_EVENTS(wuxc::TextBox, HexInputTextBox, KeyDown, LostFocus);

		TEMPLATE_PART_2_EVENTS(wuxc::Border, CheckeredBackground1Border, Loaded, InvertedPropertyChanged);
		TEMPLATE_PART_2_EVENTS(wuxc::Border, CheckeredBackground2Border, Loaded, InvertedPropertyChanged);
		TEMPLATE_PART_2_EVENTS(wuxc::Border, CheckeredBackground3Border, Loaded, InvertedPropertyChanged);
		TEMPLATE_PART_2_EVENTS(wuxc::Border, CheckeredBackground4Border, Loaded, InvertedPropertyChanged);
		TEMPLATE_PART_2_EVENTS(wuxc::Border, CheckeredBackground5Border, Loaded, InvertedPropertyChanged);
		TEMPLATE_PART_2_EVENTS(wuxc::Border, CheckeredBackground6Border, Loaded, InvertedPropertyChanged);
		TEMPLATE_PART_2_EVENTS(wuxc::Border, CheckeredBackground7Border, Loaded, InvertedPropertyChanged);
		TEMPLATE_PART_2_EVENTS(wuxc::Border, CheckeredBackground8Border, Loaded, InvertedPropertyChanged);
		TEMPLATE_PART_2_EVENTS(wuxc::Border, CheckeredBackground9Border, Loaded, InvertedPropertyChanged);
		TEMPLATE_PART_2_EVENTS(wuxc::Border, CheckeredBackground10Border, Loaded, InvertedPropertyChanged);

		TEMPLATE_PART_2_EVENTS(muxc::Primitives::ColorSpectrum, ColorSpectrumControl, ColorChanged, GotFocus);

		TEMPLATE_PART_2_EVENTS(wuxc::Primitives::ToggleButton, HsvToggleButton, Checked, Unchecked);
		TEMPLATE_PART_2_EVENTS(wuxc::Primitives::ToggleButton, RgbToggleButton, Checked, Unchecked);

		TEMPLATE_PART_1_EVENT(wuxc::Border, P1PreviewBorder, PointerPressed);
		TEMPLATE_PART_1_EVENT(wuxc::Border, P2PreviewBorder, PointerPressed);
		TEMPLATE_PART_1_EVENT(wuxc::Border, N1PreviewBorder, PointerPressed);
		TEMPLATE_PART_1_EVENT(wuxc::Border, N2PreviewBorder, PointerPressed);

#undef TEMPLATE_PART_2_EVENTS
#undef TEMPLATE_PART_1_EVENT

		bool m_IsInitialized = false;
		bool m_EventsConnected = false;
		void ConnectEvents(bool connected);

		event_token m_LoadedToken, m_UnloadedToken;
		void OnLoaded(const IInspectable &sender, const wux::RoutedEventArgs &args);
		void OnUnloaded(const IInspectable &sender, const wux::RoutedEventArgs &args);

		template<txmp::ColorChannel channel>
		void OnChannelSliderValueChanged(const IInspectable &sender, const wuxc::Primitives::RangeBaseValueChangedEventArgs &args);
		void OnSpectrumAlphaChannelSliderValueChanged(const IInspectable &sender, const wuxc::Primitives::RangeBaseValueChangedEventArgs &args);
		void OnThirdDimensionChannelSliderValueChanged(const IInspectable &sender, const wuxc::Primitives::RangeBaseValueChangedEventArgs &args);

		template<txmp::ColorChannel channel>
		void OnChannelSliderLoaded(const IInspectable &sender, const wux::RoutedEventArgs &args);
		void OnSpectrumAlphaChannelSliderLoaded(const IInspectable &sender, const wux::RoutedEventArgs &args);
		void OnThirdDimensionChannelSliderLoaded(const IInspectable &sender, const wux::RoutedEventArgs &args);

		template<txmp::ColorChannel channel>
		void OnChannelTextBoxKeyDown(const IInspectable &sender, const wux::Input::KeyRoutedEventArgs &args);

		template<txmp::ColorChannel channel>
		void OnChannelTextBoxLostFocus(const IInspectable &sender, const wux::RoutedEventArgs &args);

		void OnHexInputTextBoxKeyDown(const IInspectable &sender, const wux::Input::KeyRoutedEventArgs &args);
		void OnHexInputTextBoxLostFocus(const IInspectable &sender, const wux::RoutedEventArgs &args);

		fire_and_forget OnCheckeredBackgroundBorderLoaded(const IInspectable &sender, const wux::RoutedEventArgs &args);
		void OnCheckeredBackgroundInvertedPropertyChanged(const wux::DependencyObject &sender, const wux::DependencyProperty &prop);

		void OnColorSpectrumControlColorChanged(const IInspectable &sender, const muxc::ColorChangedEventArgs &args);
		void OnColorSpectrumControlGotFocus(const IInspectable &sender, const wux::RoutedEventArgs &args);

		template<txmp::ColorRepresentation representation>
		void OnColorRepresentationToggleButtonCheckedChanged(const IInspectable &sender, const wux::RoutedEventArgs &args);

		void OnPreviewBorderPointerPressed(const IInspectable &sender, const wux::Input::IPointerRoutedEventArgs &args);

		event_token m_ColorPropertyChangedToken;
		static void OnColorPropertyChanged(const wux::DependencyObject &sender, const wux::DependencyProperty &prop);

		event_token m_DispatcherQueueTimerTickToken;
		void OnDispatcherQueueTimerTick(const IInspectable &sender, const IInspectable &args);

		// revoker is OK here because we don't need to hold a strong ref
		wux::XamlRoot::Changed_revoker m_XamlRootChangedRevoker;
		void OnXamlRootChanged(const wux::XamlRoot &sender, const wux::XamlRootChangedEventArgs &args);

		// store ready to use event handlers to avoid needing to reallocate those all the time.
#define EVENT_HANDLER(TYPE, NAME, FUNCTION) \
		TYPE NAME = { get_weak(), &ColorPicker::FUNCTION };

		EVENT_HANDLER(wuxc::Primitives::RangeBaseValueChangedEventHandler, m_Channel1SliderValueChangedHandler, OnChannelSliderValueChanged<txmp::ColorChannel::Channel1>);
		EVENT_HANDLER(wuxc::Primitives::RangeBaseValueChangedEventHandler, m_Channel2SliderValueChangedHandler, OnChannelSliderValueChanged<txmp::ColorChannel::Channel2>);
		EVENT_HANDLER(wuxc::Primitives::RangeBaseValueChangedEventHandler, m_Channel3SliderValueChangedHandler, OnChannelSliderValueChanged<txmp::ColorChannel::Channel3>);
		EVENT_HANDLER(wuxc::Primitives::RangeBaseValueChangedEventHandler, m_AlphaChannelSliderValueChangedHandler, OnChannelSliderValueChanged<txmp::ColorChannel::Alpha>);
		EVENT_HANDLER(wuxc::Primitives::RangeBaseValueChangedEventHandler, m_SpectrumAlphaChannelSliderValueChangedHandler, OnSpectrumAlphaChannelSliderValueChanged);
		EVENT_HANDLER(wuxc::Primitives::RangeBaseValueChangedEventHandler, m_ThirdDimensionSliderValueChangedHandler, OnThirdDimensionChannelSliderValueChanged);

		EVENT_HANDLER(wux::RoutedEventHandler, m_Channel1SliderLoadedHandler, OnChannelSliderLoaded<txmp::ColorChannel::Channel1>);
		EVENT_HANDLER(wux::RoutedEventHandler, m_Channel2SliderLoadedHandler, OnChannelSliderLoaded<txmp::ColorChannel::Channel2>);
		EVENT_HANDLER(wux::RoutedEventHandler, m_Channel3SliderLoadedHandler, OnChannelSliderLoaded<txmp::ColorChannel::Channel3>);
		EVENT_HANDLER(wux::RoutedEventHandler, m_AlphaChannelSliderLoadedHandler, OnChannelSliderLoaded<txmp::ColorChannel::Alpha>);
		EVENT_HANDLER(wux::RoutedEventHandler, m_SpectrumAlphaChannelSliderLoadedHandler, OnSpectrumAlphaChannelSliderLoaded);
		EVENT_HANDLER(wux::RoutedEventHandler, m_ThirdDimensionSliderLoadedHandler, OnThirdDimensionChannelSliderLoaded);

		EVENT_HANDLER(wux::Input::KeyEventHandler, m_Channel1TextBoxKeyDownHandler, OnChannelTextBoxKeyDown<txmp::ColorChannel::Channel1>);
		EVENT_HANDLER(wux::Input::KeyEventHandler, m_Channel2TextBoxKeyDownHandler, OnChannelTextBoxKeyDown<txmp::ColorChannel::Channel2>);
		EVENT_HANDLER(wux::Input::KeyEventHandler, m_Channel3TextBoxKeyDownHandler, OnChannelTextBoxKeyDown<txmp::ColorChannel::Channel3>);
		EVENT_HANDLER(wux::Input::KeyEventHandler, m_AlphaChannelTextBoxKeyDownHandler, OnChannelTextBoxKeyDown<txmp::ColorChannel::Alpha>);
		EVENT_HANDLER(wux::Input::KeyEventHandler, m_HexInputTextBoxKeyDownHandler, OnHexInputTextBoxKeyDown);

		EVENT_HANDLER(wux::RoutedEventHandler, m_Channel1TextBoxLostFocusHandler, OnChannelTextBoxLostFocus<txmp::ColorChannel::Channel1>);
		EVENT_HANDLER(wux::RoutedEventHandler, m_Channel2TextBoxLostFocusHandler, OnChannelTextBoxLostFocus<txmp::ColorChannel::Channel2>);
		EVENT_HANDLER(wux::RoutedEventHandler, m_Channel3TextBoxLostFocusHandler, OnChannelTextBoxLostFocus<txmp::ColorChannel::Channel3>);
		EVENT_HANDLER(wux::RoutedEventHandler, m_AlphaChannelTextBoxLostFocusHandler, OnChannelTextBoxLostFocus<txmp::ColorChannel::Alpha>);
		EVENT_HANDLER(wux::RoutedEventHandler, m_HexInputTextBoxLostFocusHandler, OnHexInputTextBoxLostFocus);

		EVENT_HANDLER(wux::RoutedEventHandler, m_CheckeredBackgroundBorderLoadedHandler, OnCheckeredBackgroundBorderLoaded);
		EVENT_HANDLER(wux::DependencyPropertyChangedCallback, m_CheckeredBackgroundBorderInvertedPropertyChangedHandler, OnCheckeredBackgroundInvertedPropertyChanged)

		EVENT_HANDLER(wux::RoutedEventHandler, m_ColorSpectrumControlGotFocusHandler, OnColorSpectrumControlGotFocus);

		EVENT_HANDLER(wux::RoutedEventHandler, m_HsvToggleButtonCheckedChangedHandler, OnColorRepresentationToggleButtonCheckedChanged<txmp::ColorRepresentation::Hsva>);
		EVENT_HANDLER(wux::RoutedEventHandler, m_RgbToggleButtonCheckedChangedHandler, OnColorRepresentationToggleButtonCheckedChanged<txmp::ColorRepresentation::Rgba>);

		EVENT_HANDLER(wux::Input::PointerEventHandler, m_PreviewBorderPointerPressedHandler, OnPreviewBorderPointerPressed);

		// These two manually declared because macros suck
		wf::TypedEventHandler<muxc::Primitives::ColorSpectrum, muxc::ColorChangedEventArgs> m_ColorSpectrumControlColorChangedHandler = { get_weak(), &ColorPicker::OnColorSpectrumControlColorChanged };
		wf::TypedEventHandler<wux::XamlRoot, wux::XamlRootChangedEventArgs> m_XamlRootChangedHandler = { get_weak(), &ColorPicker::OnXamlRootChanged };

#undef EVENT_HANDLER

		void ApplyChannelTextBoxValue(const wuxc::TextBox &channelTextBox, txmp::ColorChannel channel);
		void UpdateColorRightNow(Windows::UI::Color newColor, bool isFromSpectrum = false);
		void ScheduleColorUpdate(Windows::UI::Color newColor, bool isFromSpectrum = false);
		void SetActiveColorRepresentation(txmp::ColorRepresentation representation);
		void SetColorChannel(txmp::ColorRepresentation representation, txmp::ColorChannel channel, double newValue);
		void UpdateVisualState(bool useTransitions);
		void UpdateColorControlValues();
		void UpdateChannelSliderBackgrounds();
		void UpdateChannelSliderBackground(const ColorPickerSlider &slider, txmp::ColorChannel channel, txmp::ColorRepresentation representation);
		void UpdateCustomPalette();

		txmp::ColorRepresentation GetActiveColorRepresentation();
		txmp::ColorChannel GetActiveColorSpectrumThirdDimension();

		static wux::DependencyProperty m_InvertedCheckerboardProperty;
	};
}

FACTORY(winrt::TranslucentTB::Xaml::Controls, ColorPicker);
