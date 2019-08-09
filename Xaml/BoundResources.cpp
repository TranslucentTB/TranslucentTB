#include "pch.h"

#include "BoundResources.h"
#if __has_include("BoundResources.g.cpp")
#include "BoundResources.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml;

namespace winrt::TranslucentTB::Xaml::implementation
{
    BoundResources::BoundResources()
    {
        InitializeComponent();
    }
}
