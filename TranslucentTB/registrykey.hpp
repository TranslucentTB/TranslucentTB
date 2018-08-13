#pragma once
#include <string>
#include <utility>
#include <winerror.h>
#include <winreg.h>
#include <winrt/base.h>

struct registry_key_traits
{
	using type = HKEY;

	static void close(type value) noexcept
	{
		if (value != HKEY_CLASSES_ROOT &&
			value != HKEY_CURRENT_CONFIG &&
			value != HKEY_CURRENT_USER &&
			value != HKEY_CURRENT_USER_LOCAL_SETTINGS &&
			value != HKEY_LOCAL_MACHINE &&
			value != HKEY_PERFORMANCE_DATA &&
			value != HKEY_PERFORMANCE_NLSTEXT &&
			value != HKEY_PERFORMANCE_TEXT &&
			value != HKEY_USERS)
		{
			WINRT_VERIFY_(ERROR_SUCCESS, RegCloseKey(value));
		}
	}

	static constexpr type invalid() noexcept
	{
		return nullptr;
	}
};

using registry_key = winrt::handle_type<registry_key_traits>;

registry_key open_key(const registry_key &key, const std::wstring &subkey)
{
	registry_key created_key;
	SetLastError(RegCreateKey(key.get(), subkey.c_str(), created_key.put()));

	return created_key;
}