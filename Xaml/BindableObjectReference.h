#pragma once
#include "dependencyproperty.h"
#include "factory.h"
#include "winrt.hpp"

#include "BindableObjectReference.g.h"

namespace winrt::TranslucentTB::Xaml::implementation
{
    struct BindableObjectReference : BindableObjectReferenceT<BindableObjectReference>
    {
        BindableObjectReference() = default;

		DECL_DEPENDENCY_PROPERTY(IInspectable, Object);
    };
}

FACTORY(winrt::TranslucentTB::Xaml, BindableObjectReference);
