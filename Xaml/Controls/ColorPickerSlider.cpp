#include "pch.h"

#include "ColorPickerSlider.h"
#if __has_include("Controls/ColorPickerSlider.g.cpp")
#include "Controls/ColorPickerSlider.g.cpp"
#endif

#include "../Converters/ContrastBrushConverter.h"
#include "util/color.hpp"
#include "colorpickerrendering.hpp"

namespace winrt::TranslucentTB::Xaml::Controls::implementation
{
	void ColorPickerSlider::OnDependencyPropertyChanged(const IInspectable &sender, const wux::DependencyPropertyChangedEventArgs &args)
	{
		if (const auto slider = sender.try_as<ColorPickerSlider>())
		{
			const auto prop = args.Property();
			if (prop == ColorProperty())
			{
				slider->HsvColor(Util::Color(slider->Color()).ToHSV());
			}
			else if (prop == CheckerBackgroundColorProperty())
			{
				slider->UpdateBackground(slider->HsvColor());
			}
			else if (slider->IsAutoUpdatingEnabled())
			{
				slider->UpdateColors();
			}
		}
	}

	ColorPickerSlider::ColorPickerSlider()
	{
		m_LoadedToken = Loaded({ get_weak(), &ColorPickerSlider::OnLoaded });
		m_UnloadedToken = Unloaded({ get_weak(), &ColorPickerSlider::OnUnloaded });
		m_OrientationPropertyChangedToken.value = RegisterPropertyChangedCallback(composable_base::OrientationProperty(), OnOrientationPropertyChanged);
		DefaultStyleKey(box_value(name_of<class_type>()));
	}

	ColorPickerSlider::~ColorPickerSlider()
	{
		UnregisterPropertyChangedCallback(composable_base::OrientationProperty(), m_OrientationPropertyChangedToken.value);
		Unloaded(m_UnloadedToken);
		Loaded(m_LoadedToken);
	}

	void ColorPickerSlider::UpdateColors()
	{
		auto hsvColor = HsvColor();

		// Calculate and set the background
		UpdateBackground(hsvColor);

		// Calculate and set the foreground ensuring contrast with the background
		Windows::UI::Color rgbColor = Util::Color::FromHSV(hsvColor);
		Windows::UI::Color selectedRgbColor;
		const double sliderPercent = Value() / (Maximum() - Minimum());

		const auto colorChannel = ColorChannel();
		const auto alphaMaxForced = IsAlphaMaxForced();
		if (ColorRepresentation() == txmp::ColorRepresentation::Hsva)
		{
			if (alphaMaxForced && colorChannel != txmp::ColorChannel::Alpha)
			{
				hsvColor.A = 1.0;
			}

			const auto saturationMaxForced = IsSaturationValueMaxForced();
			switch (colorChannel)
			{
			case txmp::ColorChannel::Channel1:
				hsvColor.H = std::clamp(sliderPercent * 360.0, 0.0, 360.0);
				if (saturationMaxForced)
				{
					hsvColor.S = hsvColor.V = 1.0;
				}
				break;

			case txmp::ColorChannel::Channel2:
				hsvColor.S = std::clamp(sliderPercent * 1.0, 0.0, 1.0);
				if (saturationMaxForced)
				{
					hsvColor.V = 1.0;
				}
				break;

			case txmp::ColorChannel::Channel3:
				hsvColor.V = std::clamp(sliderPercent * 1.0, 0.0, 1.0);
				if (saturationMaxForced)
				{
					hsvColor.S = 1.0;
				}
				break;
			}

			selectedRgbColor = Util::Color::FromHSV(hsvColor);
		}
		else
		{
			if (alphaMaxForced && colorChannel != txmp::ColorChannel::Alpha)
			{
				rgbColor.A = 255;
			}

			const auto channelValue = static_cast<uint8_t>(std::clamp(sliderPercent * 255.0, 0.0, 255.0));
			switch (colorChannel)
			{
			case txmp::ColorChannel::Channel1:
				rgbColor.R = channelValue;
				break;

			case txmp::ColorChannel::Channel2:
				rgbColor.G = channelValue;
				break;

			case txmp::ColorChannel::Channel3:
				rgbColor.B = channelValue;
				break;
			}

			selectedRgbColor = rgbColor;
		}

		wux::VisualStateManager::GoToState(*this, Util::Color(selectedRgbColor).IsDarkColor() ? L"LightSliderThumb" : L"DarkSliderThumb", false);
	}

	void ColorPickerSlider::OnApplyTemplate()
	{
		m_HorizontalTrackRect = GetTemplateChild(L"HorizontalTrackRect").try_as<wux::Shapes::Rectangle>();
		m_VerticalTrackRect = GetTemplateChild(L"VerticalTrackRect").try_as<wux::Shapes::Rectangle>();
		base_type::OnApplyTemplate();

		UpdateVisualState();
		UpdateBackground(HsvColor());
	}

	wf::Size ColorPickerSlider::MeasureOverride(const wf::Size &availableSize)
	{
		if (availableSize != oldSize)
		{
			measuredSize = base_type::MeasureOverride(availableSize);
			oldSize = availableSize;
		}

		return measuredSize;
	}

	void ColorPickerSlider::OnOrientationPropertyChanged(const wux::DependencyObject &sender, const wux::DependencyProperty &)
	{
		if (const auto that = sender.try_as<ColorPickerSlider>())
		{
			that->UpdateVisualState();
			that->UpdateBackground(that->HsvColor());
		}
	}

	void ColorPickerSlider::OnLoaded(const IInspectable &, const wux::RoutedEventArgs &)
	{
		if (const auto root = XamlRoot())
		{
			m_XamlRootChangedRevoker = root.Changed(winrt::auto_revoke, m_XamlRootChangedHandler);
			OnXamlRootChanged(nullptr, nullptr);
		}
	}

	void ColorPickerSlider::OnUnloaded(const IInspectable &, const wux::RoutedEventArgs &)
	{
		m_XamlRootChangedRevoker.revoke();
	}

	void ColorPickerSlider::OnXamlRootChanged(const wux::XamlRoot &, const wux::XamlRootChangedEventArgs &)
	{
		UpdateBackground(HsvColor());
	}

	void ColorPickerSlider::UpdateVisualState()
	{
		wux::VisualStateManager::GoToState(*this, Orientation() == wuxc::Orientation::Vertical ? L"VerticalColorSlider" : L"HorizontalColorSlider", false);
	}

	void ColorPickerSlider::UpdateBackground(txmp::HsvColor color)
	{
		const auto orientation = Orientation();
		if (const auto rect = orientation == wuxc::Orientation::Vertical ? m_VerticalTrackRect : m_HorizontalTrackRect)
		{
			auto width = static_cast<int32_t>(rect.ActualWidth());
			auto height = static_cast<int32_t>(rect.ActualHeight());

			if (width == 0 || height == 0)
			{
				if (cachedSize != wf::Size { })
				{
					width = static_cast<int32_t>(cachedSize.Width);
					height = static_cast<int32_t>(cachedSize.Height);
				}
			}
			else
			{
				cachedSize = { static_cast<float>(width), static_cast<float>(height) };
			}

			if (width != 0 && height != 0)
			{
				double renderScale = 1.0;
				if (const auto root = rect.XamlRoot())
				{
					renderScale = root.RasterizationScale();

					width = static_cast<int32_t>(width * renderScale);
					height = static_cast<int32_t>(height * renderScale);
				}

				RenderBackground(rect, color, orientation, width, height, renderScale);
			}
		}
	}

	fire_and_forget ColorPickerSlider::RenderBackground(weak_ref<wux::Shapes::Rectangle> rect_weak, txmp::HsvColor color, wuxc::Orientation orientation, int32_t width, int32_t height, double renderScale)
	{
		// retrieve properties before going to the background
		const auto representation = ColorRepresentation();
		const auto channel = ColorChannel();
		const auto background = CheckerBackgroundColor();
		const auto alphaMaxForced = IsAlphaMaxForced();
		const auto saturationMaxForced = IsSaturationValueMaxForced();

		// create the bitmap on this thread
		wux::Media::Imaging::WriteableBitmap bitmap { width, height };
		const auto buffer = bitmap.PixelBuffer();
		const auto bgraPixelData = buffer.data();

		// move to background
		apartment_context context;
		co_await resume_background();

		FillChannelBitmap(bgraPixelData, width, height, renderScale, orientation, representation, channel, color, background, alphaMaxForced, saturationMaxForced);

		if (const auto rect_strong = rect_weak.get())
		{
			// return to main thread
			co_await context;

			bitmap.Invalidate();
			wux::Media::ImageBrush brush;
			brush.ImageSource(bitmap);
			brush.Stretch(wux::Media::Stretch::Fill);
			rect_strong.Fill(brush);
		}
	}
}
