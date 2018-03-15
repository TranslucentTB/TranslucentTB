#pragma once
#include <objbase.h>
#include <memory>

template<typename T>
struct MemDeleter {

	void operator()(T *data)
	{
		CoTaskMemFree(data);
	}

};

template<typename T>
using AutoMemFree = std::unique_ptr<T, MemDeleter<T>>;