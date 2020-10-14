#pragma once

#include "Controls/ChromeButton.g.h"
#include "..\dependencyproperty.h"

// FIXME: ugly workaround for https://github.com/microsoft/microsoft-ui-xaml/issues/2429
using namespace winrt::TranslucentTB::Xaml::implementation;

namespace winrt::TranslucentTB::Xaml::Controls::implementation
{
	struct ChromeButton : ChromeButtonT<ChromeButton>
	{
		ChromeButton();

		DECL_REF_DEPENDENCY_PROPERTY(Windows::UI::Xaml::Controls::IconElement, Icon);
		DECL_VALUE_DEPENDENCY_PROPERTY(Windows::UI::Color, NormalForeground);
		DECL_VALUE_DEPENDENCY_PROPERTY(Windows::UI::Color, NormalBackground);
		DECL_VALUE_DEPENDENCY_PROPERTY(Windows::UI::Color, HoverForeground);
		DECL_VALUE_DEPENDENCY_PROPERTY(Windows::UI::Color, HoverBackground);
		DECL_VALUE_DEPENDENCY_PROPERTY(Windows::UI::Color, PressedForeground);
		DECL_VALUE_DEPENDENCY_PROPERTY(Windows::UI::Color, PressedBackground);
		DECL_VALUE_DEPENDENCY_PROPERTY(bool, IsTogglable);

		void OnToggle();
	};
}

namespace winrt::TranslucentTB::Xaml::Controls::factory_implementation
{
	struct ChromeButton : ChromeButtonT<ChromeButton, implementation::ChromeButton>
	{
	};
}
