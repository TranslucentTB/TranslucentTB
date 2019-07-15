#pragma once

#include "App.g.h"
#include "App.base.hpp"

namespace winrt::TranslucentTB::implementation
{
	struct App : AppT2<App>
	{
		App();
	};
}

namespace winrt::TranslucentTB::factory_implementation
{
	struct App : AppT<App, implementation::App>
	{
	};
}
