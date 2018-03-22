#pragma once
#include "AutoFreeBase.hpp"
#include "ttberror.hpp"



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