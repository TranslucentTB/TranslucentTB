#pragma once
#include "../dependencyproperty.h"
#include "../factory.h"
#include "winrt.hpp"

#include "Controls/Case.g.h"

namespace winrt::TranslucentTB::Xaml::Controls::implementation
{
	struct Case : CaseT<Case>
	{
		Case() = default;

		DECL_DEPENDENCY_PROPERTY(wux::UIElement, Content);
		DECL_DEPENDENCY_PROPERTY_WITH_DEFAULT(bool, IsDefault, box_value(false));
		DECL_DEPENDENCY_PROPERTY(IInspectable, Value);
	};
}

FACTORY(winrt::TranslucentTB::Xaml::Controls, Case);
