#pragma once

#include "BoundResources.g.h"

namespace winrt::TranslucentTB::Xaml::implementation
{
	struct BoundResources : BoundResourcesT<BoundResources>
	{
		BoundResources();
	};
}

namespace winrt::TranslucentTB::Xaml::factory_implementation
{
	struct BoundResources : BoundResourcesT<BoundResources, implementation::BoundResources>
	{
	};
}
