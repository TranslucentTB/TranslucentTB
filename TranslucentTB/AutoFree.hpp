#pragma once
#include <functional>
#include <WinBase.h>
#include <winerror.h>

#include "ttberror.hpp"

void LocalFreeDeleter(void *data)
{
	if (LocalFree(data))
	{
		Error::Handle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Log, L"Failed to free memory.");
	}
}

template<typename T>
class AutoFree {

private:
	T *m_DataPtr;
	std::function<void(T *)> m_Deleter;

public:
	AutoFree(T *data = nullptr, std::function<void(T *)> deleter = LocalFreeDeleter)
	{
		m_DataPtr = data;
		m_Deleter = deleter;
	}

	T *operator =(T *data)
	{
		return m_DataPtr = data;
	}

	T **operator &()
	{
		m_Deleter(m_DataPtr);
		return &m_DataPtr;
	}

	operator T *()
	{
		return m_DataPtr;
	}

	~AutoFree()
	{
		m_Deleter(m_DataPtr);
	}

};