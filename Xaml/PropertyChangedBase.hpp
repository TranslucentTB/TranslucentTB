#pragma once
#include <type_traits>
#include "winrt.hpp"
#include <winrt/Windows.UI.Xaml.Data.h>

#include "event.h"
#include "util/string_macros.hpp"

#define PROPERTY_CHANGED_FIELD(NAME) m_ ## NAME ## Property_
#define DECL_PROPERTY_CHANGED_FUNCS(TYPE, NAME, FIELD) \
	TYPE NAME() const noexcept(std::is_nothrow_copy_constructible_v<TYPE>) \
	{ \
		return FIELD; \
	}\
	\
	void NAME(const TYPE &value) \
	{ \
		compare_assign(FIELD, value, UTIL_STRINGIFY(NAME)); \
	}

#define DECL_PROPERTY_CHANGED_PROP(TYPE, NAME) \
private: \
	TYPE PROPERTY_CHANGED_FIELD(NAME); \
	\
public: \
	DECL_PROPERTY_CHANGED_FUNCS(TYPE, NAME, PROPERTY_CHANGED_FIELD(NAME))

class PropertyChangedBase {
public:
	DECL_EVENT(wux::Data::PropertyChangedEventHandler, PropertyChanged, m_propertyChanged);

protected:
	template<typename Self, typename U>
	void compare_assign(this Self &&self, U &value, const U &new_value, std::wstring_view name)
	{
		if (value != new_value)
		{
			value = new_value;
			self.m_propertyChanged(self, wux::Data::PropertyChangedEventArgs(name));
		}
	}
};
