#include "pch.h"

#include "ChromeButton.h"
#if __has_include("Controls/ChromeButton.g.cpp")
#include "Controls/ChromeButton.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI;
using namespace Windows::UI::Xaml;

namespace winrt::TranslucentTB::Xaml::Controls::implementation
{
	ChromeButton::ChromeButton()
	{
		InitializeComponent();
	}

	void ChromeButton::OnToggle()
	{
		if (IsTogglable())
		{
			base_type::OnToggle();
		}
	}
}
