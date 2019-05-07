#pragma once
#include "arch.h"
#include <string>
#include <windef.h>

class DetourException {
private:
	LONG m_ErrorCode;
	std::wstring m_ErrorMessage;

public:
	inline DetourException(LONG errCode, std::wstring message) : m_ErrorCode(errCode), m_ErrorMessage(std::move(message)) { }

	inline LONG code() const
	{
		return m_ErrorCode;
	}

	inline const std::wstring &message() const
	{
		return m_ErrorMessage;
	}
};