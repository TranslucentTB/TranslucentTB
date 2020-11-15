#pragma once
#include "factory.h"

#include "App.g.h"
#include "App.base.hpp"

namespace winrt::TranslucentTB::Xaml::implementation
{
	struct App : AppT2<App>
	{
		App();
	};
}

FACTORY(winrt::TranslucentTB::Xaml, App);
