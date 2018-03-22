#pragma once
#include <objbase.h>
#include <WinBase.h>
#include <winerror.h>

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