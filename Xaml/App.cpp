#include "pch.h"

#include "App.h"
#if __has_include("App.g.cpp")
#include "App.g.cpp"
#endif

#include "appinfo.hpp"

using namespace winrt;
using namespace Windows::UI::Xaml;

namespace winrt::TranslucentTB::Xaml::implementation
{
	App::App()
	{
		Initialize();
	}
}
