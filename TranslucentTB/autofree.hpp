#pragma once
#include "arch.h"
#include <objbase.h>
#include <WinBase.h>
#include <winerror.h>

#include "ttberror.hpp"

class AutoFree {

private:
	template<typename T, typename Deleter>
	class Base {

	private:
		T * m_DataPtr;
		Deleter m_Deleter;

	public:
		explicit inline Base(T *data = nullptr)
		{
			m_DataPtr = data;
		}

		inline Base(const Base &) = delete;

		inline T *operator =(T *data)
		{
			m_Deleter(m_DataPtr);
			return m_DataPtr = data;
		}

		inline Base &operator =(const Base &) = delete;

		inline T **operator &()
		{
			m_Deleter(m_DataPtr);
			return &m_DataPtr;
		}

		inline operator T *()
		{
			return m_DataPtr;
		}

		inline T *data()
		{
			return m_DataPtr;
		}

		inline const T *data() const
		{
			return m_DataPtr;
		}

		inline ~Base()
		{
			m_Deleter(m_DataPtr);
		}

	};

	template<bool silent>
	struct LocalFreeDeleter {

		inline void operator() (void *data)
		{
			void *result = LocalFree(data);
			if (result && !silent)
			{
				LastErrorHandle(Error::Level::Log, L"Failed to free memory.");
			}
		}

	};

	struct CoTaskMemFreeDeleter {

		inline void operator() (void *data)
		{
			CoTaskMemFree(data);
		}

	};

	struct GlobalFreeDeleter {

		inline void operator() (void *data)
		{
			void *result = GlobalFree(data);
			if (result)
			{
				LastErrorHandle(Error::Level::Log, L"Failed to free memory.");
			}
		}

	};

public:
	template<typename T = void>
	using CoTaskMem = Base<T, CoTaskMemFreeDeleter>;

	template<typename T = void>
	using Local = Base<T, LocalFreeDeleter<false>>;

	template<typename T = void>
	using Global = Base<T, GlobalFreeDeleter>;

	// Only use this if you don't want to log it for a very valid reason.
	template<typename T = void>
	using SilentLocal = Base<T, LocalFreeDeleter<true>>;

};