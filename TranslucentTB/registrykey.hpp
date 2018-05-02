#pragma once
#include <winerror.h>
#include <winreg.h>

#include "ttberror.hpp"

class RegistryKey {

private:
	HKEY m_Key;
	bool m_Result;

public:
	inline RegistryKey(const HKEY &key, const std::wstring &subKey)
	{
		m_Result = ErrorHandle(HRESULT_FROM_WIN32(RegCreateKey(key, subKey.c_str(), &m_Key)), Error::Level::Error, L"Opening registry key failed!");
	}
	inline operator HKEY() { return m_Key; }
	inline operator bool() { return m_Result; }
	inline ~RegistryKey()
	{
		if (m_Result)
		{
			ErrorHandle(HRESULT_FROM_WIN32(RegCloseKey(m_Key)), Error::Level::Log, L"Error closing registry key.");
		}
	}
};