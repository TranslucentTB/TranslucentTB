#pragma once
#include "arch.h"
#include <oleauto.h>
#include <string>
#include <string_view>
#include <utility>

#include "../ttberror.hpp"

class SysString {
private:
	class PutProxy {
	private:
		SysString &m_strRef;
		inline PutProxy(SysString &str) : m_strRef(str) { }
		friend class SysString;

	public:
		inline operator BSTR *() { m_strRef.Release(); return &m_strRef.m_str; }
		~PutProxy() { m_strRef.AddRef(); }
	};

	BSTR m_str;

	inline void AddRef()
	{
		if (m_str)
		{
			ErrorHandle(SysAddRefString(m_str), Error::Level::Debug, L"Failed to increase string ref count");
		}
	}

	inline void Release()
	{
		if (m_str)
		{
			SysReleaseString(m_str);
			SysFreeString(m_str);
		}
	}

public:
	inline SysString() : m_str(nullptr) { }

	inline SysString(const SysString &other) : m_str(other.m_str)
	{
		AddRef();
	}

	inline SysString(SysString &&other) noexcept : m_str(std::exchange(other.m_str, nullptr)) { }

	inline SysString(std::wstring_view val) : m_str(SysAllocStringLen(val.data(), val.length()))
	{
		AddRef();
	}

	inline PutProxy put()
	{
		return *this;
	}

	inline SysString &operator =(const SysString &other)
	{
		if (this != &other)
		{
			m_str = other.m_str;
			AddRef();
		}
	}

	inline SysString &operator =(SysString &&other) noexcept
	{
		if (this != &other)
		{
			m_str = std::exchange(other.m_str, nullptr);
		}
	}

	inline explicit operator bool() { return m_str; }

	inline operator std::wstring_view()
	{
		if (m_str)
		{
			return { m_str, SysStringLen(m_str) };
		}
		else
		{
			return { };
		}
	}

	inline ~SysString()
	{
		Release();
	}
};