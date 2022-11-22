#include "pch.h"

#include "ColorPicker.h"
#if __has_include("Controls/ColorPicker.g.cpp")
#include "Controls/ColorPicker.g.cpp"
#endif

#include "colorpickerrendering.hpp"
#include "util/color.hpp"
#include "util/string_macros.hpp"

namespace winrt::TranslucentTB::Xaml::Controls::implementation
{
	wux::DependencyProperty ColorPicker::m_InvertedCheckerboardProperty =
		wux::DependencyProperty::RegisterAttached(
			L"InvertedCheckerboard",
			xaml_typename<bool>(),
			xaml_typename<TranslucentTB::Xaml::Controls::ColorPicker>(),
			wux::PropertyMetadata(box_value(false)));

	void ColorPicker::OnDependencyPropertyChanged(const IInspectable &sender, const wux::DependencyPropertyChangedEventArgs &args)
	{
		if (const auto that = sender.try_as<ColorPicker>())
		{
			const auto property = args.Property();
			if (property == CustomPaletteProperty())
			{
				that->UpdateCustomPalette();
			}
			else if (property == IsColorPaletteVisibleProperty())
			{
				that->UpdateVisualState(false);
			}
			else if (property == CheckerBackgroundColorProperty())
			{
				that->OnXamlRootChanged(nullptr, nullptr);
			}
		}
	}

	ColorPicker::ColorPicker()
	{
		m_DecimalFormatter.FractionDigits(0);
		SetValue(CustomPaletteColorsProperty(), single_threaded_observable_vector<Windows::UI::Color>());
		m_LoadedToken = Loaded({ get_weak(), &ColorPicker::OnLoaded });
		m_UnloadedToken = Unloaded({ get_weak(), &ColorPicker::OnUnloaded });
		DefaultStyleKey(box_value(name_of<class_type>()));

		m_ColorPropertyChangedToken.value = RegisterPropertyChangedCallback(composable_base::ColorProperty(), OnColorPropertyChanged);

		UpdateCustomPalette();

		static constexpr int COLOR_UPDATE_INTERVAL = 30;
		m_DispatcherQueueTimer.Interval(std::chrono::milliseconds(COLOR_UPDATE_INTERVAL));
		m_DispatcherQueueTimerTickToken = m_DispatcherQueueTimer.Tick({ get_weak(), &ColorPicker::OnDispatcherQueueTimerTick });
		m_DispatcherQueueTimer.Start();
	}

	ColorPicker::~ColorPicker()
	{
		ConnectEvents(false);

		m_DispatcherQueueTimer.Tick(m_DispatcherQueueTimerTickToken);
		m_DispatcherQueueTimer.Stop();

		UnregisterPropertyChangedCallback(composable_base::ColorProperty(), m_ColorPropertyChangedToken.value);
		Unloaded(m_UnloadedToken);
		Loaded(m_LoadedToken);
	}

	void ColorPicker::OnApplyTemplate()
	{
		ConnectEvents(false);

#define GET_TEMPLATE_CHILD(NAME) m_ ## NAME = GetTemplateChild(UTIL_STRINGIFY(NAME)).try_as<decltype(m_ ## NAME)>()

		GET_TEMPLATE_CHILD(AlphaChannelSlider);
		GET_TEMPLATE_CHILD(AlphaChannelTextBox);
		GET_TEMPLATE_CHILD(Channel1Slider);
		GET_TEMPLATE_CHILD(Channel1TextBox);
		GET_TEMPLATE_CHILD(Channel2Slider);
		GET_TEMPLATE_CHILD(Channel2TextBox);
		GET_TEMPLATE_CHILD(Channel3Slider);
		GET_TEMPLATE_CHILD(Channel3TextBox);
		GET_TEMPLATE_CHILD(CheckeredBackground1Border);
		GET_TEMPLATE_CHILD(CheckeredBackground2Border);
		GET_TEMPLATE_CHILD(CheckeredBackground3Border);
		GET_TEMPLATE_CHILD(CheckeredBackground4Border);
		GET_TEMPLATE_CHILD(CheckeredBackground5Border);
		GET_TEMPLATE_CHILD(CheckeredBackground6Border);
		GET_TEMPLATE_CHILD(CheckeredBackground7Border);
		GET_TEMPLATE_CHILD(CheckeredBackground8Border);
		GET_TEMPLATE_CHILD(CheckeredBackground9Border);
		GET_TEMPLATE_CHILD(CheckeredBackground10Border);
		GET_TEMPLATE_CHILD(ColorSpectrumControl);
		GET_TEMPLATE_CHILD(ColorSpectrumAlphaSlider);
		GET_TEMPLATE_CHILD(ColorSpectrumThirdDimensionSlider);
		GET_TEMPLATE_CHILD(HexInputTextBox);
		GET_TEMPLATE_CHILD(HsvToggleButton);
		GET_TEMPLATE_CHILD(RgbToggleButton);
		GET_TEMPLATE_CHILD(P1PreviewBorder);
		GET_TEMPLATE_CHILD(P2PreviewBorder);
		GET_TEMPLATE_CHILD(N1PreviewBorder);
		GET_TEMPLATE_CHILD(N2PreviewBorder);

#undef GET_TEMPLATE_CHILD

		base_type::OnApplyTemplate();
		UpdateVisualState(false);
		SetActiveColorRepresentation(txmp::ColorRepresentation::Rgba);
		m_IsInitialized = true;
		UpdateColorControlValues();
		ConnectEvents(true);
	}

	void ColorPicker::ConnectEvents(bool connected)
	{
		if (connected && !m_EventsConnected)
		{
#define CONNECT_EVENT(NAME, EVENT, HANDLER) NAME ## EVENT ## Token = NAME.EVENT(HANDLER)

			if (m_Channel1Slider)
			{
				CONNECT_EVENT(m_Channel1Slider, ValueChanged, m_Channel1SliderValueChangedHandler);
				CONNECT_EVENT(m_Channel1Slider, Loaded, m_Channel1SliderLoadedHandler);
			}

			if (m_Channel2Slider)
			{
				CONNECT_EVENT(m_Channel2Slider, ValueChanged, m_Channel2SliderValueChangedHandler);
				CONNECT_EVENT(m_Channel2Slider, Loaded, m_Channel2SliderLoadedHandler);
			}

			if (m_Channel3Slider)
			{
				CONNECT_EVENT(m_Channel3Slider, ValueChanged, m_Channel3SliderValueChangedHandler);
				CONNECT_EVENT(m_Channel3Slider, Loaded, m_Channel3SliderLoadedHandler);
			}

			if (m_AlphaChannelSlider)
			{
				CONNECT_EVENT(m_AlphaChannelSlider, ValueChanged, m_AlphaChannelSliderValueChangedHandler);
				CONNECT_EVENT(m_AlphaChannelSlider, Loaded, m_AlphaChannelSliderLoadedHandler);
			}

			if (m_ColorSpectrumAlphaSlider)
			{
				CONNECT_EVENT(m_ColorSpectrumAlphaSlider, ValueChanged, m_SpectrumAlphaChannelSliderValueChangedHandler);
				CONNECT_EVENT(m_ColorSpectrumAlphaSlider, Loaded, m_SpectrumAlphaChannelSliderLoadedHandler);
			}

			if (m_ColorSpectrumThirdDimensionSlider)
			{
				CONNECT_EVENT(m_ColorSpectrumThirdDimensionSlider, ValueChanged, m_ThirdDimensionSliderValueChangedHandler);
				CONNECT_EVENT(m_ColorSpectrumThirdDimensionSlider, Loaded, m_ThirdDimensionSliderLoadedHandler);
			}

			if (m_Channel1TextBox)
			{
				CONNECT_EVENT(m_Channel1TextBox, KeyDown, m_Channel1TextBoxKeyDownHandler);
				CONNECT_EVENT(m_Channel1TextBox, LostFocus, m_Channel1TextBoxLostFocusHandler);
			}

			if (m_Channel2TextBox)
			{
				CONNECT_EVENT(m_Channel2TextBox, KeyDown, m_Channel2TextBoxKeyDownHandler);
				CONNECT_EVENT(m_Channel2TextBox, LostFocus, m_Channel2TextBoxLostFocusHandler);
			}

			if (m_Channel3TextBox)
			{
				CONNECT_EVENT(m_Channel3TextBox, KeyDown, m_Channel3TextBoxKeyDownHandler);
				CONNECT_EVENT(m_Channel3TextBox, LostFocus, m_Channel3TextBoxLostFocusHandler);
			}

			if (m_AlphaChannelTextBox)
			{
				CONNECT_EVENT(m_AlphaChannelTextBox, KeyDown, m_AlphaChannelTextBoxKeyDownHandler);
				CONNECT_EVENT(m_AlphaChannelTextBox, LostFocus, m_AlphaChannelTextBoxLostFocusHandler);
			}

			if (m_HexInputTextBox)
			{
				CONNECT_EVENT(m_HexInputTextBox, KeyDown, m_HexInputTextBoxKeyDownHandler);
				CONNECT_EVENT(m_HexInputTextBox, LostFocus, m_HexInputTextBoxLostFocusHandler);
			}

#define CONNECT_INVERTED_PROPERTY_HANDLER(NAME) \
	NAME ## InvertedPropertyChangedToken.value = NAME.RegisterPropertyChangedCallback(m_InvertedCheckerboardProperty, m_CheckeredBackgroundBorderInvertedPropertyChangedHandler);

			if (m_CheckeredBackground1Border)
			{
				CONNECT_EVENT(m_CheckeredBackground1Border, Loaded, m_CheckeredBackgroundBorderLoadedHandler);
				CONNECT_INVERTED_PROPERTY_HANDLER(m_CheckeredBackground1Border);
			}

			if (m_CheckeredBackground2Border)
			{
				CONNECT_EVENT(m_CheckeredBackground2Border, Loaded, m_CheckeredBackgroundBorderLoadedHandler);
				CONNECT_INVERTED_PROPERTY_HANDLER(m_CheckeredBackground2Border);
			}

			if (m_CheckeredBackground3Border)
			{
				CONNECT_EVENT(m_CheckeredBackground3Border, Loaded, m_CheckeredBackgroundBorderLoadedHandler);
				CONNECT_INVERTED_PROPERTY_HANDLER(m_CheckeredBackground3Border);
			}

			if (m_CheckeredBackground4Border)
			{
				CONNECT_EVENT(m_CheckeredBackground4Border, Loaded, m_CheckeredBackgroundBorderLoadedHandler);
				CONNECT_INVERTED_PROPERTY_HANDLER(m_CheckeredBackground4Border);
			}

			if (m_CheckeredBackground5Border)
			{
				CONNECT_EVENT(m_CheckeredBackground5Border, Loaded, m_CheckeredBackgroundBorderLoadedHandler);
				CONNECT_INVERTED_PROPERTY_HANDLER(m_CheckeredBackground5Border);
			}

			if (m_CheckeredBackground6Border)
			{
				CONNECT_EVENT(m_CheckeredBackground6Border, Loaded, m_CheckeredBackgroundBorderLoadedHandler);
				CONNECT_INVERTED_PROPERTY_HANDLER(m_CheckeredBackground6Border);
			}

			if (m_CheckeredBackground7Border)
			{
				CONNECT_EVENT(m_CheckeredBackground7Border, Loaded, m_CheckeredBackgroundBorderLoadedHandler);
				CONNECT_INVERTED_PROPERTY_HANDLER(m_CheckeredBackground7Border);
			}

			if (m_CheckeredBackground8Border)
			{
				CONNECT_EVENT(m_CheckeredBackground8Border, Loaded, m_CheckeredBackgroundBorderLoadedHandler);
				CONNECT_INVERTED_PROPERTY_HANDLER(m_CheckeredBackground8Border);
			}

			if (m_CheckeredBackground9Border)
			{
				CONNECT_EVENT(m_CheckeredBackground9Border, Loaded, m_CheckeredBackgroundBorderLoadedHandler);
				CONNECT_INVERTED_PROPERTY_HANDLER(m_CheckeredBackground9Border);
			}

			if (m_CheckeredBackground10Border)
			{
				CONNECT_EVENT(m_CheckeredBackground10Border, Loaded, m_CheckeredBackgroundBorderLoadedHandler);
				CONNECT_INVERTED_PROPERTY_HANDLER(m_CheckeredBackground10Border);
			}

#undef CONNECT_INVERTED_PROPERTY_HANDLER

			if (m_ColorSpectrumControl)
			{
				CONNECT_EVENT(m_ColorSpectrumControl, ColorChanged, m_ColorSpectrumControlColorChangedHandler);
				CONNECT_EVENT(m_ColorSpectrumControl, GotFocus, m_ColorSpectrumControlGotFocusHandler);
			}

			if (m_HsvToggleButton)
			{
				CONNECT_EVENT(m_HsvToggleButton, Checked, m_HsvToggleButtonCheckedChangedHandler);
				CONNECT_EVENT(m_HsvToggleButton, Unchecked, m_HsvToggleButtonCheckedChangedHandler);
			}

			if (m_RgbToggleButton)
			{
				CONNECT_EVENT(m_RgbToggleButton, Checked, m_RgbToggleButtonCheckedChangedHandler);
				CONNECT_EVENT(m_RgbToggleButton, Unchecked, m_RgbToggleButtonCheckedChangedHandler);
			}

			if (m_P1PreviewBorder)
			{
				CONNECT_EVENT(m_P1PreviewBorder, PointerPressed, m_PreviewBorderPointerPressedHandler);
			}

			if (m_P2PreviewBorder)
			{
				CONNECT_EVENT(m_P2PreviewBorder, PointerPressed, m_PreviewBorderPointerPressedHandler);
			}

			if (m_N1PreviewBorder)
			{
				CONNECT_EVENT(m_N1PreviewBorder, PointerPressed, m_PreviewBorderPointerPressedHandler);
			}

			if (m_N2PreviewBorder)
			{
				CONNECT_EVENT(m_N2PreviewBorder, PointerPressed, m_PreviewBorderPointerPressedHandler);
			}

#undef CONNECT_EVENT

			m_EventsConnected = true;
		}
		else if (!connected && m_EventsConnected)
		{
#define DISCONNECT_EVENT(NAME, EVENT) NAME.EVENT(NAME ## EVENT ## Token)

			if (m_Channel1Slider)
			{
				DISCONNECT_EVENT(m_Channel1Slider, ValueChanged);
				DISCONNECT_EVENT(m_Channel1Slider, Loaded);
			}

			if (m_Channel2Slider)
			{
				DISCONNECT_EVENT(m_Channel2Slider, ValueChanged);
				DISCONNECT_EVENT(m_Channel2Slider, Loaded);
			}

			if (m_Channel3Slider)
			{
				DISCONNECT_EVENT(m_Channel3Slider, ValueChanged);
				DISCONNECT_EVENT(m_Channel3Slider, Loaded);
			}

			if (m_AlphaChannelSlider)
			{
				DISCONNECT_EVENT(m_AlphaChannelSlider, ValueChanged);
				DISCONNECT_EVENT(m_AlphaChannelSlider, Loaded);
			}

			if (m_ColorSpectrumAlphaSlider)
			{
				DISCONNECT_EVENT(m_ColorSpectrumAlphaSlider, ValueChanged);
				DISCONNECT_EVENT(m_ColorSpectrumAlphaSlider, Loaded);
			}

			if (m_ColorSpectrumThirdDimensionSlider)
			{
				DISCONNECT_EVENT(m_ColorSpectrumThirdDimensionSlider, ValueChanged);
				DISCONNECT_EVENT(m_ColorSpectrumThirdDimensionSlider, Loaded);
			}

			if (m_Channel1TextBox)
			{
				DISCONNECT_EVENT(m_Channel1TextBox, KeyDown);
				DISCONNECT_EVENT(m_Channel1TextBox, LostFocus);
			}

			if (m_Channel2TextBox)
			{
				DISCONNECT_EVENT(m_Channel2TextBox, KeyDown);
				DISCONNECT_EVENT(m_Channel2TextBox, LostFocus);
			}

			if (m_Channel3TextBox)
			{
				DISCONNECT_EVENT(m_Channel3TextBox, KeyDown);
				DISCONNECT_EVENT(m_Channel3TextBox, LostFocus);
			}

			if (m_AlphaChannelTextBox)
			{
				DISCONNECT_EVENT(m_AlphaChannelTextBox, KeyDown);
				DISCONNECT_EVENT(m_AlphaChannelTextBox, LostFocus);
			}

			if (m_HexInputTextBox)
			{
				DISCONNECT_EVENT(m_HexInputTextBox, KeyDown);
				DISCONNECT_EVENT(m_HexInputTextBox, LostFocus);
			}

#define DISCONNECT_INVERTED_PROPERTY_HANDLER(NAME) \
	NAME.UnregisterPropertyChangedCallback(m_InvertedCheckerboardProperty, NAME ## InvertedPropertyChangedToken.value)

			if (m_CheckeredBackground1Border)
			{
				DISCONNECT_EVENT(m_CheckeredBackground1Border, Loaded);
				DISCONNECT_INVERTED_PROPERTY_HANDLER(m_CheckeredBackground1Border);
			}

			if (m_CheckeredBackground2Border)
			{
				DISCONNECT_EVENT(m_CheckeredBackground2Border, Loaded);
				DISCONNECT_INVERTED_PROPERTY_HANDLER(m_CheckeredBackground2Border);
			}

			if (m_CheckeredBackground3Border)
			{
				DISCONNECT_EVENT(m_CheckeredBackground3Border, Loaded);
				DISCONNECT_INVERTED_PROPERTY_HANDLER(m_CheckeredBackground3Border);
			}

			if (m_CheckeredBackground4Border)
			{
				DISCONNECT_EVENT(m_CheckeredBackground4Border, Loaded);
				DISCONNECT_INVERTED_PROPERTY_HANDLER(m_CheckeredBackground4Border);
			}

			if (m_CheckeredBackground5Border)
			{
				DISCONNECT_EVENT(m_CheckeredBackground5Border, Loaded);
				DISCONNECT_INVERTED_PROPERTY_HANDLER(m_CheckeredBackground5Border);
			}

			if (m_CheckeredBackground6Border)
			{
				DISCONNECT_EVENT(m_CheckeredBackground6Border, Loaded);
				DISCONNECT_INVERTED_PROPERTY_HANDLER(m_CheckeredBackground6Border);
			}

			if (m_CheckeredBackground7Border)
			{
				DISCONNECT_EVENT(m_CheckeredBackground7Border, Loaded);
				DISCONNECT_INVERTED_PROPERTY_HANDLER(m_CheckeredBackground7Border);
			}

			if (m_CheckeredBackground8Border)
			{
				DISCONNECT_EVENT(m_CheckeredBackground8Border, Loaded);
				DISCONNECT_INVERTED_PROPERTY_HANDLER(m_CheckeredBackground8Border);
			}

			if (m_CheckeredBackground9Border)
			{
				DISCONNECT_EVENT(m_CheckeredBackground9Border, Loaded);
				DISCONNECT_INVERTED_PROPERTY_HANDLER(m_CheckeredBackground9Border);
			}

			if (m_CheckeredBackground10Border)
			{
				DISCONNECT_EVENT(m_CheckeredBackground10Border, Loaded);
				DISCONNECT_INVERTED_PROPERTY_HANDLER(m_CheckeredBackground10Border);
			}

#undef DISCONNECT_INVERTED_PROPERTY_HANDLER

			if (m_ColorSpectrumControl)
			{
				DISCONNECT_EVENT(m_ColorSpectrumControl, ColorChanged);
				DISCONNECT_EVENT(m_ColorSpectrumControl, GotFocus);
			}

			if (m_HsvToggleButton)
			{
				DISCONNECT_EVENT(m_HsvToggleButton, Checked);
				DISCONNECT_EVENT(m_HsvToggleButton, Unchecked);
			}

			if (m_RgbToggleButton)
			{
				DISCONNECT_EVENT(m_RgbToggleButton, Checked);
				DISCONNECT_EVENT(m_RgbToggleButton, Unchecked);
			}

			if (m_P1PreviewBorder)
			{
				DISCONNECT_EVENT(m_P1PreviewBorder, PointerPressed);
			}

			if (m_P2PreviewBorder)
			{
				DISCONNECT_EVENT(m_P2PreviewBorder, PointerPressed);
			}

			if (m_N1PreviewBorder)
			{
				DISCONNECT_EVENT(m_N1PreviewBorder, PointerPressed);
			}

			if (m_N2PreviewBorder)
			{
				DISCONNECT_EVENT(m_N2PreviewBorder, PointerPressed);
			}

#undef DISCONNECT_EVENT

			m_EventsConnected = false;
		}
	}

	void ColorPicker::OnLoaded(const IInspectable &, const wux::RoutedEventArgs &)
	{
		if (const auto root = XamlRoot())
		{
			m_XamlRootChangedRevoker = root.Changed(winrt::auto_revoke, m_XamlRootChangedHandler);
			OnXamlRootChanged(nullptr, nullptr);
		}
	}

	void ColorPicker::OnUnloaded(const IInspectable &, const wux::RoutedEventArgs &)
	{
		m_XamlRootChangedRevoker.revoke();
	}

	template<txmp::ColorChannel channel>
	void ColorPicker::OnChannelSliderValueChanged(const IInspectable &, const wuxc::Primitives::RangeBaseValueChangedEventArgs &args)
	{
		SetColorChannel(GetActiveColorRepresentation(), channel, args.NewValue());
	}

	void ColorPicker::OnSpectrumAlphaChannelSliderValueChanged(const IInspectable &, const wuxc::Primitives::RangeBaseValueChangedEventArgs &args)
	{
		SetColorChannel(txmp::ColorRepresentation::Hsva, txmp::ColorChannel::Alpha, args.NewValue());
	}

	void ColorPicker::OnThirdDimensionChannelSliderValueChanged(const IInspectable &, const wuxc::Primitives::RangeBaseValueChangedEventArgs &args)
	{
		SetColorChannel(txmp::ColorRepresentation::Hsva, GetActiveColorSpectrumThirdDimension(), args.NewValue());
	}

	template<txmp::ColorChannel channel>
	void ColorPicker::OnChannelSliderLoaded(const IInspectable &sender, const wux::RoutedEventArgs &)
	{
		UpdateChannelSliderBackground(sender.try_as<ColorPickerSlider>(), channel, GetActiveColorRepresentation());
	}

	void ColorPicker::OnSpectrumAlphaChannelSliderLoaded(const IInspectable &, const wux::RoutedEventArgs &)
	{
		UpdateChannelSliderBackground(m_ColorSpectrumAlphaSlider, txmp::ColorChannel::Alpha, txmp::ColorRepresentation::Hsva);
	}

	void ColorPicker::OnThirdDimensionChannelSliderLoaded(const IInspectable &, const wux::RoutedEventArgs &)
	{
		UpdateChannelSliderBackground(m_ColorSpectrumThirdDimensionSlider, GetActiveColorSpectrumThirdDimension(), txmp::ColorRepresentation::Hsva);
	}

	template<txmp::ColorChannel channel>
	void ColorPicker::OnChannelTextBoxKeyDown(const IInspectable &sender, const wux::Input::KeyRoutedEventArgs &args)
	{
		if (args.Key() == Windows::System::VirtualKey::Enter)
		{
			ApplyChannelTextBoxValue(sender.try_as<wuxc::TextBox>(), channel);
		}
	}

	template<txmp::ColorChannel channel>
	void ColorPicker::OnChannelTextBoxLostFocus(const IInspectable &sender, const wux::RoutedEventArgs &)
	{
		ApplyChannelTextBoxValue(sender.try_as<wuxc::TextBox>(), channel);
	}

	void ColorPicker::OnHexInputTextBoxKeyDown(const IInspectable &, const wux::Input::KeyRoutedEventArgs &args)
	{
		if (args.Key() == Windows::System::VirtualKey::Enter)
		{
			try
			{
				UpdateColorRightNow(Util::Color::FromString(m_HexInputTextBox.Text(), true));
			}
			catch (...)
			{
				UpdateColorControlValues();
				UpdateChannelSliderBackgrounds();
			}
		}
	}

	void ColorPicker::OnHexInputTextBoxLostFocus(const IInspectable &, const wux::RoutedEventArgs &)
	{
		try
		{
			UpdateColorRightNow(Util::Color::FromString(m_HexInputTextBox.Text(), true));
		}
		catch (...)
		{
			UpdateColorControlValues();
			UpdateChannelSliderBackgrounds();
		}
	}

	fire_and_forget ColorPicker::OnCheckeredBackgroundBorderLoaded(const IInspectable &sender, const wux::RoutedEventArgs &args)
	{
		if (auto border = sender.try_as<wuxc::Border>())
		{
			auto width = static_cast<int32_t>(border.ActualWidth());
			auto height = static_cast<int32_t>(border.ActualHeight());

			if (width != 0 && height != 0)
			{
				// retrieve properties before going to the background
				const auto background = CheckerBackgroundColor();
				const bool inverted = GetInvertedCheckerboard(border);

				double renderScale = 1.0;
				if (const auto root = border.XamlRoot())
				{
					renderScale = root.RasterizationScale();

					width = static_cast<int32_t>(width * renderScale);
					height = static_cast<int32_t>(height * renderScale);
				}

				// create the bitmap on this thread
				wux::Media::Imaging::WriteableBitmap bitmap { width, height };
				const auto buffer = bitmap.PixelBuffer();
				const auto bgraPixelData = buffer.data();

				// move to background
				const weak_ref<wuxc::Border> border_weak = border;
				border = nullptr;
				const auto dispatcher = Windows::System::DispatcherQueue::GetForCurrentThread();
				co_await resume_background();

				FillCheckeredBitmap(bgraPixelData, width, height, renderScale, background, inverted);

				if (const auto border_strong = border_weak.get())
				{
					// return to main thread
					co_await wil::resume_foreground(dispatcher);

					bitmap.Invalidate();
					wux::Media::ImageBrush brush;
					brush.ImageSource(bitmap);
					brush.Stretch(wux::Media::Stretch::Fill);
					border_strong.Background(brush);
				}
			}
		}
	}

	void ColorPicker::OnCheckeredBackgroundInvertedPropertyChanged(const wux::DependencyObject &sender, const wux::DependencyProperty &)
	{
		OnCheckeredBackgroundBorderLoaded(sender, nullptr);
	}

	void ColorPicker::OnColorSpectrumControlColorChanged(const IInspectable &, const muxc::ColorChangedEventArgs &args)
	{
		// It is OK in this case to use the RGB representation
		ScheduleColorUpdate(args.NewColor(), true);
	}

	void ColorPicker::OnColorSpectrumControlGotFocus(const IInspectable &, const wux::RoutedEventArgs &)
	{
		Windows::UI::Color rgbColor = m_ColorSpectrumControl.Color();

		/* If this control has a color that is currently empty (#00000000),
		 * selecting a new color directly in the spectrum will fail. This is
		 * a bug in the color spectrum. Selecting a new color in the spectrum will
		 * keep zero for all channels (including alpha and the third dimension).
		 *
		 * In practice this means a new color cannot be selected using the spectrum
		 * until both the alpha and third dimension slider are raised above zero.
		 * This is extremely user unfriendly and must be corrected as best as possible.
		 *
		 * In order to work around this, detect when the color spectrum has selected
		 * a new color and then automatically set the alpha and third dimension
		 * channel to maximum. However, the color spectrum has a second bug, the
		 * ColorChanged event is never raised if the color is empty. This prevents
		 * automatically setting the other channels where it normally should be done
		 * (in the ColorChanged event).
		 *
		 * In order to work around this second bug, the GotFocus event is used
		 * to detect when the spectrum is engaged by the user. It's somewhat equivalent
		 * to ColorChanged for this purpose. Then when the GotFocus event is fired
		 * set the alpha and third channel values to maximum. The problem here is that
		 * the GotFocus event does not have access to the new color that was selected
		 * in the spectrum. It is not available due to the afore mentioned bug or due to
		 * timing. This means the best that can be done is to just set a 'neutral'
		 * color such as white.
		 *
		 * There is still a small usability issue with this as it requires two
		 * presses to set a color. That's far better than having to slide up both
		 * sliders though.
		 *
		 *  1. If the color is empty, the first press on the spectrum will set white
		 *     and ignore the pressed color on the spectrum
		 *  2. The second press on the spectrum will be correctly handled.
		 *
		 */

		if (rgbColor == Windows::UI::Color { })
		{
			ScheduleColorUpdate({ 0xFF, 0xFF, 0xFF, 0xFF });
		}
		else if (rgbColor.A == 0x00)
		{
			// As an additional usability improvement, reset alpha to maximum when the spectrum is used.
			// The color spectrum has no alpha channel and it is much more intuitive to do this for the user
			// especially if the picker was initially set with Colors.Transparent.
			rgbColor.A = 0xFF;
			ScheduleColorUpdate(rgbColor);
		}
	}

	template<txmp::ColorRepresentation representation>
	void ColorPicker::OnColorRepresentationToggleButtonCheckedChanged(const IInspectable &, const wux::RoutedEventArgs &)
	{
		SetActiveColorRepresentation(representation);
		UpdateColorControlValues();
		UpdateChannelSliderBackgrounds();
	}

	void ColorPicker::OnPreviewBorderPointerPressed(const IInspectable &sender, const wux::Input::IPointerRoutedEventArgs &)
	{
		if (const auto border = sender.try_as<wuxc::Border>())
		{
			if (const auto brush = border.Background().try_as<wux::Media::SolidColorBrush>())
			{
				ScheduleColorUpdate(brush.Color());
			}
		}
	}

	void ColorPicker::OnColorPropertyChanged(const wux::DependencyObject &sender, const wux::DependencyProperty &)
	{
		if (const auto that = sender.try_as<ColorPicker>())
		{
			// TODO: Coerce the value if Alpha is disabled, is this handled in the base ColorPicker?
			if (that->m_SavedHsvColor && that->Color() != that->m_SavedHsvColorRgbEquivalent)
			{
				// The color was updated from an unknown source
				// The RGB and HSV colors are no longer in sync so the HSV color must be cleared
				that->m_SavedHsvColor.reset();
				that->m_SavedHsvColorRgbEquivalent.reset();
			}

			that->UpdateColorControlValues();
			that->UpdateChannelSliderBackgrounds();
		}
	}

	void ColorPicker::OnDispatcherQueueTimerTick(const IInspectable &, const IInspectable &)
	{
		if (m_UpdatedRgbColor)
		{
			const auto newColor = *m_UpdatedRgbColor;

			// Clear first to avoid timing issues if it takes longer than the timer interval to set the new color
			m_UpdatedRgbColor.reset();

			// An equality check here is important
			// Without it, OnColorChanged would continuously be invoked and preserveHsvColor overwritten when not wanted
			if (newColor != Color())
			{
				// Disable events here so the color update isn't repeated as other controls in the UI are updated through binding.
				// For example, the Spectrum should be bound to Color, as soon as Color is changed here the Spectrum is updated.
				// Then however, the ColorSpectrum.ColorChanged event would fire which would schedule a new color update --
				// with the same color. This causes several problems:
				//   1. Layout cycle that may crash the app
				//   2. A performance hit recalculating for no reason
				//   3. preserveHsvColor gets overwritten unexpectedly by the ColorChanged handler
				ConnectEvents(false);
				UpdateColorRightNow(newColor, m_UpdateFromSpectrum);
				ConnectEvents(true);
			}

			m_UpdateFromSpectrum = false;
		}
	}

	void ColorPicker::OnXamlRootChanged(const wux::XamlRoot &, const wux::XamlRootChangedEventArgs &)
	{
		OnCheckeredBackgroundBorderLoaded(m_CheckeredBackground1Border, nullptr);
		OnCheckeredBackgroundBorderLoaded(m_CheckeredBackground2Border, nullptr);
		OnCheckeredBackgroundBorderLoaded(m_CheckeredBackground3Border, nullptr);
		OnCheckeredBackgroundBorderLoaded(m_CheckeredBackground4Border, nullptr);
		OnCheckeredBackgroundBorderLoaded(m_CheckeredBackground5Border, nullptr);
		OnCheckeredBackgroundBorderLoaded(m_CheckeredBackground6Border, nullptr);
		OnCheckeredBackgroundBorderLoaded(m_CheckeredBackground7Border, nullptr);
		OnCheckeredBackgroundBorderLoaded(m_CheckeredBackground8Border, nullptr);
		OnCheckeredBackgroundBorderLoaded(m_CheckeredBackground9Border, nullptr);
		OnCheckeredBackgroundBorderLoaded(m_CheckeredBackground10Border, nullptr);
	}

	void ColorPicker::ApplyChannelTextBoxValue(const wuxc::TextBox &channelTextBox, txmp::ColorChannel channel)
	{
		if (channelTextBox)
		{
			const auto text = channelTextBox.Text();
			if (Util::Trim(text).empty())
			{
				// An empty string is allowed and happens when the clear TextBox button is pressed
				// This case should be interpreted as zero
				SetColorChannel(GetActiveColorRepresentation(), channel, 0.0);
			}
			else if (auto parsed = m_DecimalFormatter.ParseDouble(text))
			{
				SetColorChannel(GetActiveColorRepresentation(), channel, parsed.Value());
			}
			else
			{
				// Reset TextBox values
				UpdateColorControlValues();
				UpdateChannelSliderBackgrounds();
			}
		}
	}

	void ColorPicker::ScheduleColorUpdate(Windows::UI::Color newColor, bool isFromSpectrum)
	{
		if (!IsAlphaEnabled())
		{
			newColor.A = 255;
		}

		m_UpdatedRgbColor = newColor;
		m_UpdateFromSpectrum = isFromSpectrum;
	}

	void ColorPicker::UpdateColorRightNow(Windows::UI::Color newColor, bool isFromSpectrum)
	{
		m_UpdateFromSpectrum = isFromSpectrum;
		Color(newColor);
	}

	void ColorPicker::SetActiveColorRepresentation(txmp::ColorRepresentation representation)
	{
		bool eventsDisconnectedByMethod = false;

		// Disable events during the update
		if (m_EventsConnected)
		{
			ConnectEvents(false);
			eventsDisconnectedByMethod = true;
		}

		// Sync the UI controls and visual state
		// The default is always RGBA
		if (representation == txmp::ColorRepresentation::Hsva)
		{
			if (m_RgbToggleButton)
			{
				const auto isChecked = m_RgbToggleButton.IsChecked();
				if (isChecked && isChecked.Value())
				{
					m_RgbToggleButton.IsChecked(false);
				}
			}

			if (m_HsvToggleButton)
			{
				const auto isChecked = m_HsvToggleButton.IsChecked();
				if (!isChecked || !isChecked.Value())
				{
					m_HsvToggleButton.IsChecked(true);
				}
			}
		}
		else
		{
			if (m_RgbToggleButton)
			{
				const auto isChecked = m_RgbToggleButton.IsChecked();
				if (!isChecked || !isChecked.Value())
				{
					m_RgbToggleButton.IsChecked(true);
				}
			}

			if (m_HsvToggleButton)
			{
				const auto isChecked = m_HsvToggleButton.IsChecked();
				if (isChecked && isChecked.Value())
				{
					m_HsvToggleButton.IsChecked(false);
				}
			}
		}

		UpdateVisualState(false);

		if (eventsDisconnectedByMethod)
		{
			ConnectEvents(true);
		}
	}

	void ColorPicker::SetColorChannel(txmp::ColorRepresentation representation, txmp::ColorChannel channel, double newValue)
	{
		Windows::UI::Color oldRgbColor = Color();
		Windows::UI::Color newRgbColor{};

		if (representation == txmp::ColorRepresentation::Hsva)
		{
			txmp::HsvColor oldHsvColor{};

			// Warning: Always maintain/use HSV information in the saved HSV color
			// This avoids loss of precision and drift caused by continuously converting to/from RGB
			if (!m_SavedHsvColor)
			{
				oldHsvColor = Util::Color(oldRgbColor).ToHSV();
			}
			else
			{
				oldHsvColor = *m_SavedHsvColor;
			}

			switch (channel)
			{
			case txmp::ColorChannel::Channel1:
				oldHsvColor.H = isnan(newValue) ? 0.0 : std::clamp(newValue, 0.0, 360.0);
				break;

			case txmp::ColorChannel::Channel2:
				oldHsvColor.S = isnan(newValue) ? 0.0 : std::clamp(newValue / 100.0, 0.0, 1.0);
				break;

			case txmp::ColorChannel::Channel3:
				oldHsvColor.V = isnan(newValue) ? 0.0 : std::clamp(newValue / 100.0, 0.0, 1.0);
				break;

			case txmp::ColorChannel::Alpha:
				// Unlike color channels, default to no transparency
				oldHsvColor.A = isnan(newValue) ? 1.0 : std::clamp(newValue / 100.0, 0.0, 1.0);
				break;
			}

			newRgbColor = Util::Color::FromHSV(oldHsvColor);

			// Must update HSV color
			m_SavedHsvColor = oldHsvColor;
			m_SavedHsvColorRgbEquivalent = newRgbColor;
		}
		else
		{
			switch (channel)
			{
			case txmp::ColorChannel::Channel1:
				oldRgbColor.R = isnan(newValue) ? 0 : static_cast<uint8_t>(std::clamp(newValue, 0.0, 255.0));
				break;

			case txmp::ColorChannel::Channel2:
				oldRgbColor.G = isnan(newValue) ? 0 : static_cast<uint8_t>(std::clamp(newValue, 0.0, 255.0));
				break;

			case txmp::ColorChannel::Channel3:
				oldRgbColor.B = isnan(newValue) ? 0 : static_cast<uint8_t>(std::clamp(newValue, 0.0, 255.0));
				break;

			case txmp::ColorChannel::Alpha:
				// Unlike color channels, default to no transparency
				oldRgbColor.A = isnan(newValue) ? 255 : static_cast<uint8_t>(std::clamp(newValue, 0.0, 255.0));
				break;
			}

			newRgbColor = oldRgbColor;

			// Must clear saved HSV color
			m_SavedHsvColor.reset();
			m_SavedHsvColorRgbEquivalent.reset();
		}

		ScheduleColorUpdate(newRgbColor);
	}

	void ColorPicker::UpdateVisualState(bool useTransitions)
	{
		wux::VisualStateManager::GoToState(*this, IsEnabled() ? L"Normal" : L"Disabled", useTransitions);
		wux::VisualStateManager::GoToState(*this, GetActiveColorRepresentation() == txmp::ColorRepresentation::Hsva ? L"HsvSelected" : L"RgbSelected", useTransitions);
		wux::VisualStateManager::GoToState(*this, IsColorPaletteVisible() ? L"ColorPaletteVisible" : L"ColorPaletteCollapsed", useTransitions);
	}

	void ColorPicker::UpdateColorControlValues()
	{
		if (m_IsInitialized)
		{
			bool eventsDisconnectedByMethod = false;
			Util::Color rgbColor = Color();

			// Disable events during the update
			if (m_EventsConnected)
			{
				ConnectEvents(false);
				eventsDisconnectedByMethod = true;
			}

			if (m_HexInputTextBox)
			{
				uint32_t colorRgba = rgbColor.ToRGBA();

				hstring colorHex;
				if (IsAlphaEnabled())
				{
					colorHex = winrt::format(L"{:08X}", colorRgba);
				}
				else
				{
					colorHex = winrt::format(L"{:06X}", static_cast<uint32_t>(colorRgba >> 8));
				}

				m_HexInputTextBox.Text(colorHex);
			}

			// Regardless of the active color representation, the spectrum is always HSV
			// Therefore, always calculate HSV color here
			// Warning: Always maintain/use HSV information in the saved HSV color
			// This avoids loss of precision and drift caused by continuously converting to/from RGB
			if (!m_SavedHsvColor)
			{
				if (m_UpdateFromSpectrum && m_ColorSpectrumControl)
				{
					// grab it from the spectrum for more precision
					m_SavedHsvColor = Util::HsvColor(m_ColorSpectrumControl.HsvColor());
				}
				else
				{
					auto hsvCol = rgbColor.ToHSV();

					// Round the channels, be sure rounding matches with the scaling next
					// Rounding of SVA requires at MINIMUM 2 decimal places
					hsvCol.H = std::round(hsvCol.H);
					hsvCol.S = std::floor(hsvCol.S * 100.0) / 100.0;
					hsvCol.V = std::floor(hsvCol.V * 100.0) / 100.0;
					hsvCol.A = std::floor(hsvCol.A * 100.0) / 100.0;

					// Must update HSV color
					m_SavedHsvColor = hsvCol;
				}

				m_SavedHsvColorRgbEquivalent = rgbColor;
			}

			const Util::HsvColor hsvColor = *m_SavedHsvColor;

			// Update the color spectrum
			// Remember the spectrum is always HSV and must be updated as such to avoid
			// conversion errors
			// Don't update HsvColor if the update came from the spectrum, because it makes the cursor "wobble"
			if (m_ColorSpectrumControl && !m_UpdateFromSpectrum)
			{
				m_ColorSpectrumControl.HsvColor(hsvColor);
			}

			// Update the color spectrum third dimension channel
			if (m_ColorSpectrumThirdDimensionSlider)
			{
				// Convert the channels into a usable range for the user
				switch (GetActiveColorSpectrumThirdDimension())
				{
				case txmp::ColorChannel::Channel1:
				{
					// Hue
					m_ColorSpectrumThirdDimensionSlider.Minimum(0.0);
					m_ColorSpectrumThirdDimensionSlider.Maximum(360.0);
					m_ColorSpectrumThirdDimensionSlider.Value(hsvColor.H);
					break;
				}

				case txmp::ColorChannel::Channel2:
				{
					// Saturation
					m_ColorSpectrumThirdDimensionSlider.Minimum(0.0);
					m_ColorSpectrumThirdDimensionSlider.Maximum(100.0);
					m_ColorSpectrumThirdDimensionSlider.Value(hsvColor.S * 100.0);
					break;
				}

				case txmp::ColorChannel::Channel3:
				{
					// Value
					m_ColorSpectrumThirdDimensionSlider.Minimum(0.0);
					m_ColorSpectrumThirdDimensionSlider.Maximum(100.0);
					m_ColorSpectrumThirdDimensionSlider.Value(hsvColor.V * 100.0);
					break;
				}
				}
			}

			// Color spectrum alpha
			if (m_ColorSpectrumAlphaSlider)
			{
				m_ColorSpectrumAlphaSlider.Minimum(0.0);
				m_ColorSpectrumAlphaSlider.Maximum(100.0);
				m_ColorSpectrumAlphaSlider.Value(hsvColor.A * 100);
			}

			// Update all other color channels
			if (GetActiveColorRepresentation() == txmp::ColorRepresentation::Hsva)
			{
				// Convert the channels into a usable range for the user
				const double saturation = hsvColor.S * 100;
				const double value = hsvColor.V * 100;
				const double alpha = hsvColor.A * 100;

				// Hue
				if (m_Channel1TextBox)
				{
					m_Channel1TextBox.MaxLength(3);
					m_Channel1TextBox.Text(m_DecimalFormatter.FormatInt(static_cast<int64_t>(hsvColor.H)));
				}

				if (m_Channel1Slider)
				{
					m_Channel1Slider.Minimum(0.0);
					m_Channel1Slider.Maximum(360.0);
					m_Channel1Slider.Value(hsvColor.H);
				}

				// Saturation
				if (m_Channel2TextBox)
				{
					m_Channel2TextBox.MaxLength(3);
					m_Channel2TextBox.Text(m_DecimalFormatter.FormatInt(static_cast<int64_t>(saturation)));
				}

				if (m_Channel2Slider)
				{
					m_Channel2Slider.Minimum(0.0);
					m_Channel2Slider.Maximum(100.0);
					m_Channel2Slider.Value(saturation);
				}

				// Value
				if (m_Channel3TextBox)
				{
					m_Channel3TextBox.MaxLength(3);
					m_Channel3TextBox.Text(m_DecimalFormatter.FormatInt(static_cast<int64_t>(value)));
				}

				if (m_Channel3Slider)
				{
					m_Channel3Slider.Minimum(0.0);
					m_Channel3Slider.Maximum(100.0);
					m_Channel3Slider.Value(value);
				}

				// Alpha
				if (m_AlphaChannelTextBox)
				{
					m_AlphaChannelTextBox.MaxLength(3);
					m_AlphaChannelTextBox.Text(m_DecimalFormatter.FormatInt(static_cast<int64_t>(alpha)));
				}

				if (m_AlphaChannelSlider)
				{
					m_AlphaChannelSlider.Minimum(0.0);
					m_AlphaChannelSlider.Maximum(100.0);
					m_AlphaChannelSlider.Value(alpha);
				}
			}
			else
			{
				// Red
				if (m_Channel1TextBox)
				{
					m_Channel1TextBox.MaxLength(3);
					m_Channel1TextBox.Text(m_DecimalFormatter.FormatInt(rgbColor.R));
				}

				if (m_Channel1Slider)
				{
					m_Channel1Slider.Minimum(0.0);
					m_Channel1Slider.Maximum(255.0);
					m_Channel1Slider.Value(rgbColor.R);
				}

				// Green
				if (m_Channel2TextBox)
				{
					m_Channel2TextBox.MaxLength(3);
					m_Channel2TextBox.Text(m_DecimalFormatter.FormatInt(rgbColor.G));
				}

				if (m_Channel2Slider)
				{
					m_Channel2Slider.Minimum(0.0);
					m_Channel2Slider.Maximum(255.0);
					m_Channel2Slider.Value(rgbColor.G);
				}

				// Blue
				if (m_Channel3TextBox)
				{
					m_Channel3TextBox.MaxLength(3);
					m_Channel3TextBox.Text(m_DecimalFormatter.FormatInt(rgbColor.B));
				}

				if (m_Channel3Slider)
				{
					m_Channel3Slider.Minimum(0.0);
					m_Channel3Slider.Maximum(255.0);
					m_Channel3Slider.Value(rgbColor.B);
				}

				// Alpha
				if (m_AlphaChannelTextBox)
				{
					m_AlphaChannelTextBox.MaxLength(3);
					m_AlphaChannelTextBox.Text(m_DecimalFormatter.FormatInt(rgbColor.A));
				}

				if (m_AlphaChannelSlider)
				{
					m_AlphaChannelSlider.Minimum(0.0);
					m_AlphaChannelSlider.Maximum(255.0);
					m_AlphaChannelSlider.Value(rgbColor.A);
				}
			}

			if (eventsDisconnectedByMethod)
			{
				ConnectEvents(true);
			}
		}
	}

	void ColorPicker::UpdateChannelSliderBackgrounds()
	{
		UpdateChannelSliderBackground(m_Channel1Slider, txmp::ColorChannel::Channel1, GetActiveColorRepresentation());
		UpdateChannelSliderBackground(m_Channel2Slider, txmp::ColorChannel::Channel2, GetActiveColorRepresentation());
		UpdateChannelSliderBackground(m_Channel3Slider, txmp::ColorChannel::Channel3, GetActiveColorRepresentation());
		UpdateChannelSliderBackground(m_AlphaChannelSlider, txmp::ColorChannel::Alpha, GetActiveColorRepresentation());

		// Always HSV
		UpdateChannelSliderBackground(m_ColorSpectrumAlphaSlider, txmp::ColorChannel::Alpha, txmp::ColorRepresentation::Hsva);
		UpdateChannelSliderBackground(m_ColorSpectrumThirdDimensionSlider, GetActiveColorSpectrumThirdDimension(), txmp::ColorRepresentation::Hsva);
	}

	void ColorPicker::UpdateChannelSliderBackground(const ColorPickerSlider &slider, txmp::ColorChannel channel, txmp::ColorRepresentation representation)
	{
		if (slider)
		{
			// Regardless of the active color representation, the sliders always use HSV
			// Therefore, always calculate HSV color here
			// Warning: Always maintain/use HSV information in the saved HSV color
			// This avoids loss of precision and drift caused by continuously converting to/from RGB
			if (!m_SavedHsvColor)
			{
				const auto rgbColor = Color();

				m_SavedHsvColor = Util::Color(rgbColor).ToHSV();
				m_SavedHsvColorRgbEquivalent = rgbColor;
			}

			slider.IsAutoUpdatingEnabled(false);
			slider.ColorChannel(channel);
			slider.ColorRepresentation(representation);
			slider.HsvColor(*m_SavedHsvColor);
			slider.UpdateColors();
		}
	}

	void ColorPicker::UpdateCustomPalette()
	{
		if (const auto palette = CustomPalette())
		{
			const auto shadeCount = palette.ShadeCount();
			const auto colorCount = palette.ColorCount();

			CustomPaletteColumnCount(colorCount);

			if (const auto colors = CustomPaletteColors())
			{
				colors.Clear();
				for (uint32_t shadeIndex = 0; shadeIndex < shadeCount; ++shadeIndex)
				{
					for (uint32_t colorIndex = 0; colorIndex < colorCount; ++colorIndex)
					{
						colors.Append(palette.GetColor(colorIndex, shadeIndex));
					}
				}
			}
		}
	}

	txmp::ColorRepresentation ColorPicker::GetActiveColorRepresentation()
	{
		if (m_HsvToggleButton)
		{
			const auto isChecked = m_HsvToggleButton.IsChecked();
			if (isChecked && isChecked.Value())
			{
				return txmp::ColorRepresentation::Hsva;
			}
		}

		return txmp::ColorRepresentation::Rgba;
	}

	txmp::ColorChannel ColorPicker::GetActiveColorSpectrumThirdDimension()
	{
		switch (ColorSpectrumComponents())
		{
		case muxc::ColorSpectrumComponents::SaturationValue:
		case muxc::ColorSpectrumComponents::ValueSaturation:
			return txmp::ColorChannel::Channel1;

		case muxc::ColorSpectrumComponents::HueValue:
		case muxc::ColorSpectrumComponents::ValueHue:
			return txmp::ColorChannel::Channel2;

		case muxc::ColorSpectrumComponents::HueSaturation:
		case muxc::ColorSpectrumComponents::SaturationHue:
			return txmp::ColorChannel::Channel3;

		// should never get there
		default: throw std::invalid_argument("ColorSpectrumComponents has an unexpected value");
		}
	}
}
