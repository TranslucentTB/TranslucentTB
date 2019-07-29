#include "pch.h"

#include "App.h"
#if __has_include("App.g.cpp")
#include "App.g.cpp"
#endif

#include "appinfo.hpp"

using namespace winrt;
using namespace Windows::UI::Xaml;

namespace winrt::TranslucentTB::implementation
{
	App::App()
	{
		Initialize();

		// Make sure the AppName resource is present, is a string and matches the APP_NAME macro.
		WINRT_ASSERT(unbox_value_or<hstring>(Resources().TryLookup(box_value(L"AppName")), L"") == APP_NAME);
	}
}
