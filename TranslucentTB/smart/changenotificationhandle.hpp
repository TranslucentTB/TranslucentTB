#pragma once
#include "arch.h"
#include <fileapi.h>
#include <windef.h>
#include <winrt/base.h>

struct change_notification_handle_traits {
	using type = HANDLE;

	inline static void close(type value) noexcept
	{
		WINRT_VERIFY(FindCloseChangeNotification(value));
	}

	inline static type invalid() noexcept
	{
		return INVALID_HANDLE_VALUE;
	}
};

using change_notification_handle = winrt::handle_type<change_notification_handle_traits>;