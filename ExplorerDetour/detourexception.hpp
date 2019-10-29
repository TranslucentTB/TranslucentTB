#pragma once
#include "arch.h"
#include <windef.h>

#include "util/null_terminated_string_view.hpp"

class DetourException {
private:
	LONG m_ErrorCode;
	Util::null_terminated_wstring_view m_ErrorMessage;

public:
	constexpr DetourException(LONG errCode, Util::null_terminated_wstring_view message) noexcept : m_ErrorCode(errCode), m_ErrorMessage(message) { }

	constexpr LONG code() const noexcept
	{
		return m_ErrorCode;
	}

	constexpr Util::null_terminated_wstring_view message() const noexcept
	{
		return m_ErrorMessage;
	}
};
