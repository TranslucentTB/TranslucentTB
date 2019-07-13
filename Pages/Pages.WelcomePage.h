#pragma once

#include "Pages.WelcomePage.g.h"

namespace winrt::TranslucentTB::Pages::implementation
{
    struct WelcomePage : WelcomePageT<WelcomePage>
    {
        WelcomePage();

        int32_t MyProperty();
        void MyProperty(int32_t value);

        void ClickHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
    };
}

namespace winrt::TranslucentTB::Pages::factory_implementation
{
    struct WelcomePage : WelcomePageT<WelcomePage, implementation::WelcomePage>
    {
    };
}
