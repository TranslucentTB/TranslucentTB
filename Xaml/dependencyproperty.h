#pragma once
#ifndef __midl
# include "util/string_macros.hpp"
# define DEPENDENCY_PROPERTY_FIELD(NAME) s_ ## NAME ## Property_
# define DECL_DEPENDENCY_PROPERTY(TYPE, NAME) \
private: \
	inline static winrt::Windows::UI::Xaml::DependencyProperty DEPENDENCY_PROPERTY_FIELD(NAME) = \
		winrt::Windows::UI::Xaml::DependencyProperty::Register( \
			UTIL_STRINGIFY(NAME), \
			winrt::xaml_typename<TYPE>(), \
			winrt::xaml_typename<class_type>(), \
			nullptr); \
	\
public: \
	static winrt::Windows::UI::Xaml::DependencyProperty NAME ## Property() noexcept \
	{ \
		return DEPENDENCY_PROPERTY_FIELD(NAME); \
	} \
	\
	TYPE NAME() \
	{ \
		return winrt::unbox_value<TYPE>(GetValue(DEPENDENCY_PROPERTY_FIELD(NAME))); \
	} \
	\
	void NAME(const TYPE &value_) \
	{ \
		SetValue(DEPENDENCY_PROPERTY_FIELD(NAME), winrt::box_value(value_)); \
	}
#else
# define DECL_DEPENDENCY_PROPERTY(TYPE, NAME) \
	TYPE NAME; \
	static Windows.UI.Xaml.DependencyProperty NAME ## Property { get; }
#endif
