#pragma once
#include "winrt.hpp"
#include <winrt/Windows.UI.Xaml.Data.h>

#include "event.h"
#include "util/string_macros.hpp"

#define PROPERTY_CHANGED_FIELD(NAME) m_ ## NAME ## Property_
#define DECL_PROPERTY_CHANGED_FUNCS(TYPE, NAME, FIELD) \
	TYPE NAME() \
	{ \
		return FIELD; \
	}\
	\
	void NAME(const TYPE &value) \
	{ \
		compare_assign(FIELD, value, UTIL_STRINGIFY(NAME));	\
	}

#define DECL_PROPERTY_CHANGED_PROP(TYPE, NAME) \
private: \
	TYPE PROPERTY_CHANGED_FIELD(NAME); \
	\
public: \
	DECL_PROPERTY_CHANGED_FUNCS(TYPE, NAME, PROPERTY_CHANGED_FIELD(NAME))

template<typename T>
class PropertyChangedBase {
public:
	DECL_EVENT_FUNCS(wux::Data::PropertyChangedEventHandler, PropertyChanged, m_propertyChanged);

protected:
	template<typename U>
	void compare_assign(U &value, const U &new_value, std::wstring_view name)
	{
		if (value != new_value)
		{
			value = new_value;
			m_propertyChanged(*static_cast<T *>(this), wux::Data::PropertyChangedEventArgs(name));
		}
	}

private:
	winrt::event<wux::Data::PropertyChangedEventHandler> m_propertyChanged;
};
