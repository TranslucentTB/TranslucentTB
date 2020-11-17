#pragma once
#include "arch.h"
#include <windef.h>
#include <winerror.h>

#include "util/null_terminated_string_view.hpp"

class DetourResult {
private:
	LONG m_ErrorCode;
	Util::null_terminated_wstring_view m_ErrorMessage;

public:
	constexpr DetourResult() noexcept : m_ErrorCode(NO_ERROR) { }
	DetourResult(Util::null_terminated_wstring_view message) noexcept : m_ErrorCode(static_cast<LONG>(GetLastError())), m_ErrorMessage(message) { }
	constexpr DetourResult(LONG errCode, Util::null_terminated_wstring_view message) noexcept : m_ErrorCode(errCode), m_ErrorMessage(message) { }

	constexpr explicit operator bool() const noexcept
	{
		return m_ErrorCode == NO_ERROR;
	}

	constexpr LONG code() const noexcept
	{
		return m_ErrorCode;
	}

	constexpr Util::null_terminated_wstring_view message() const noexcept
	{
		return m_ErrorMessage;
	}
};
