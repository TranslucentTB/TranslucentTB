#pragma once
#define _X86_
#include "AutoFreeBase.hpp"
#include <objbase.h>
#include <WinBase.h>
#include <winerror.h>

#include "ttberror.hpp"



struct LocalFreeDeleter {

	inline void operator() (void *data)
	{
		if (LocalFree(data))
		{
			ErrorHandle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Log, L"Failed to free memory.");
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