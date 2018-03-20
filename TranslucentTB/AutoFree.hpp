#pragma once
#include <objbase.h>
#include <WinBase.h>
#include <winerror.h>

#include "ttberror.hpp"

template<typename T, typename Deleter>
class AutoFreeBase {

private:
	T *m_DataPtr;
	Deleter m_Deleter;

public:
	inline AutoFreeBase(T *data = nullptr)
	{
		m_DataPtr = data;
	}

	inline AutoFreeBase(const AutoFreeBase &) = delete;

	inline T *operator =(T *data)
	{
		m_Deleter(m_DataPtr);
		return m_DataPtr = data;
	}

	inline AutoFreeBase &operator =(const AutoFreeBase &) = delete;

	inline T **operator &()
	{
		m_Deleter(m_DataPtr);
		return &m_DataPtr;
	}

	inline operator T *()
	{
		return m_DataPtr;
	}

	inline ~AutoFreeBase()
	{
		m_Deleter(m_DataPtr);
	}

};



struct LocalFreeDeleter {

	inline void operator() (void *data)
	{
		if (LocalFree(data))
		{
			Error::Handle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Log, L"Failed to free memory.");
		}
	}

};

template<typename T = void>
using AutoLocalFree = AutoFreeBase<T, LocalFreeDeleter>;



struct CoTaskMemFreeDeleter {

	inline void operator() (void *data)
	{
		CoTaskMemFree(data);
	}

};

template<typename T = void>
using AutoCoTaskMemFree = AutoFreeBase<T, CoTaskMemFreeDeleter>;