#pragma once
#include <winrt/base.h>
#include <winrt/Windows.UI.Xaml.Data.h>

#include "util/strings.hpp"

#define PROP_NAME UTIL_WIDEN(__FUNCTION__)

template<typename T>
class PropertyChangedBase {
public:
	winrt::event_token PropertyChanged(const winrt::Windows::UI::Xaml::Data::PropertyChangedEventHandler &value)
	{
		return m_propertyChanged.add(value);
	}

	void PropertyChanged(const winrt::event_token &token)
	{
		m_propertyChanged.remove(token);
	}

protected:
	template<typename U>
	void compare_assign(U &value, const U &new_value, std::wstring_view name)
	{
		if (value != new_value)
		{
			value = new_value;
			name.remove_prefix(name.find_last_of(L':') + 1);
			m_propertyChanged(*static_cast<T *>(this), winrt::Windows::UI::Xaml::Data::PropertyChangedEventArgs(name));
		}
	}

private:
	winrt::event<winrt::Windows::UI::Xaml::Data::PropertyChangedEventHandler> m_propertyChanged;
};
