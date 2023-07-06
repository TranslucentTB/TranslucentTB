#pragma once
#include "arch.h"
#include <cstdint>
#include <windef.h>
#include <wil/win32_helpers.h>

#include "constants.hpp"
#include "util/null_terminated_string_view.hpp"
#include "windows/window.hpp"

namespace Localization {
	static constexpr Util::null_terminated_wstring_view FAILED_LOADING = L"[error occurred while loading localized string]";
	Util::null_terminated_wstring_view LoadLocalizedResourceString(uint16_t resource, HINSTANCE hInst = wil::GetModuleInstanceHandle(), WORD lang = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL));

	std::thread ShowLocalizedMessageBox(uint16_t resource, UINT type, HINSTANCE hInst = wil::GetModuleInstanceHandle());

	template<typename... Args>
	std::thread ShowLocalizedMessageBox(uint16_t resource, UINT type, HINSTANCE hInst, Args&&... args)
	{
		const auto msg = LoadLocalizedResourceString(resource, hInst);
		std::wstring formatted = std::vformat(msg, std::make_wformat_args(std::forward<Args>(args)...));

		return std::thread([formatted_msg = std::move(formatted), type]() noexcept
		{
			MessageBoxEx(Window::NullWindow, formatted_msg.c_str(), APP_NAME, type, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL));
		});
	}

	template<typename Callback>
	std::thread ShowLocalizedMessageBoxWithCallback(uint16_t resource, UINT type, HINSTANCE hInst, Callback&& callback)
	{
		const auto msg = LoadLocalizedResourceString(resource, hInst);

		return std::thread([msg, type, callback = std::forward<Callback>(callback)]() mutable noexcept(std::is_nothrow_invocable_v<Callback, int>)
		{
			const int ret = MessageBoxEx(Window::NullWindow, msg.c_str(), APP_NAME, type, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL));
			std::invoke(std::forward<Callback>(callback), ret);
		});
	}
}
