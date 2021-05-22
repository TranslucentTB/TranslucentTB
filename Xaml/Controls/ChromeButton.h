#pragma once
#include "../dependencyproperty.h"
#include "../factory.h"
#include "winrt.hpp"

#include "Controls/ChromeButton.g.h"

// FIXME: ugly workaround for https://github.com/microsoft/microsoft-ui-xaml/issues/2429
using namespace winrt::TranslucentTB::Xaml::implementation;

namespace winrt::TranslucentTB::Xaml::Controls::implementation
{
	struct ChromeButton : ChromeButtonT<ChromeButton>
	{
		ChromeButton();

		DECL_DEPENDENCY_PROPERTY(wuxc::IconElement, Icon);
		DECL_DEPENDENCY_PROPERTY(Windows::UI::Color, NormalForeground);
		DECL_DEPENDENCY_PROPERTY(Windows::UI::Color, NormalBackground);
		DECL_DEPENDENCY_PROPERTY(Windows::UI::Color, HoverForeground);
		DECL_DEPENDENCY_PROPERTY(Windows::UI::Color, HoverBackground);
		DECL_DEPENDENCY_PROPERTY(Windows::UI::Color, PressedForeground);
		DECL_DEPENDENCY_PROPERTY(Windows::UI::Color, PressedBackground);
		DECL_DEPENDENCY_PROPERTY_WITH_DEFAULT(bool, IsTogglable, box_value(false));

		void OnToggle();
	};
}

FACTORY(winrt::TranslucentTB::Xaml::Controls, ChromeButton);
