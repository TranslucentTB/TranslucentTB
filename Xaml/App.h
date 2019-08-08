#pragma once

#include "App.g.h"
#include "App.base.hpp"

namespace winrt::TranslucentTB::Xaml::implementation
{
	struct App : AppT2<App>
	{
		App();
	};
}

namespace winrt::TranslucentTB::Xaml::factory_implementation
{
	struct App : AppT<App, implementation::App>
	{
	};
}
