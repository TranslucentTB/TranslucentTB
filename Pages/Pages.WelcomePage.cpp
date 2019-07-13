#include "pch.h"

#include "Pages.WelcomePage.h"
#if __has_include("Pages.WelcomePage.g.cpp")
#include "Pages.WelcomePage.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml;

namespace winrt::TranslucentTB::Pages::implementation
{
    WelcomePage::WelcomePage()
    {
        InitializeComponent();
    }

    int32_t WelcomePage::MyProperty()
    {
        throw hresult_not_implemented();
    }

    void WelcomePage::MyProperty(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }

    void WelcomePage::ClickHandler(IInspectable const&, RoutedEventArgs const&)
    {
        Button().Content(box_value(L"Clicked"));
    }
}
