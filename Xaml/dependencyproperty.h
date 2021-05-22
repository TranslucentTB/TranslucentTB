#pragma once
#ifndef __midl
# include "winrt.hpp"
# include <winrt/Windows.UI.Xaml.h>
# include "util/string_macros.hpp"
# define DEPENDENCY_PROPERTY_FIELD(NAME) s_ ## NAME ## Property_
# define DECL_DEPENDENCY_PROPERTY_FUNCS(TYPE, NAME, FIELD) \
	static wux::DependencyProperty NAME ## Property() noexcept \
	{ \
		return FIELD; \
	} \
	\
	TYPE NAME() \
	{ \
		return winrt::unbox_value<TYPE>(GetValue(FIELD)); \
	} \
	\
	void NAME(const TYPE &value_) \
	{ \
		SetValue(FIELD, winrt::box_value(value_)); \
	}
# define DECL_DEPENDENCY_PROPERTY_FIELD(TYPE, NAME, METADATA) \
	inline static wux::DependencyProperty DEPENDENCY_PROPERTY_FIELD(NAME) = \
		wux::DependencyProperty::Register( \
			UTIL_STRINGIFY(NAME), \
			winrt::xaml_typename<TYPE>(), \
			winrt::xaml_typename<class_type>(), \
			METADATA);
# define DECL_DEPENDENCY_PROPERTY(TYPE, NAME) \
private: \
	DECL_DEPENDENCY_PROPERTY_FIELD(TYPE, NAME, nullptr) \
	\
public: \
	DECL_DEPENDENCY_PROPERTY_FUNCS(TYPE, NAME, DEPENDENCY_PROPERTY_FIELD(NAME))
# define DECL_DEPENDENCY_PROPERTY_WITH_DEFAULT(TYPE, NAME, DEFAULT) \
private: \
	DECL_DEPENDENCY_PROPERTY_FIELD(TYPE, NAME, wux::PropertyMetadata(DEFAULT)) \
	\
public: \
	DECL_DEPENDENCY_PROPERTY_FUNCS(TYPE, NAME, DEPENDENCY_PROPERTY_FIELD(NAME))
#else
# define DECL_DEPENDENCY_PROPERTY(TYPE, NAME) \
	TYPE NAME; \
	static Windows.UI.Xaml.DependencyProperty NAME ## Property { get; }
#endif
