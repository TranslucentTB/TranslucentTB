#pragma once
#include <memory>
#include <WinBase.h>
#include <winerror.h>

#include "ttberror.hpp"

template<typename T>
struct LocalDeleter {

	void operator()(T *data)
	{
		if (LocalFree(data))
		{
			Error::Handle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Log, L"Failed to free memory.");
		}
	}

};

template<typename T>
using AutoFree = std::unique_ptr<T, LocalDeleter<T>>;