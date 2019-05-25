#pragma once
#include "arch.h"
#include <objbase.h>
#include <type_traits>
#include <utility>
#include <WinBase.h>
#include <winerror.h>

#include "ttberror.hpp"

class AutoFree {
protected:
	template<typename T, class traits>
	class BaseImpl {
	protected:
		T *m_DataPtr;

		inline BaseImpl(void *data) : m_DataPtr(static_cast<T *>(data)) { }

	public:
		inline BaseImpl(T *&&data = nullptr) : m_DataPtr(data) { }

		inline BaseImpl(BaseImpl &&other) noexcept : m_DataPtr(other.detach())
		{
			return;
		}

		inline BaseImpl(const BaseImpl &) = delete;

		inline BaseImpl &operator =(BaseImpl &&other)
		{
			if (this != &other)
			{
				traits::close(m_DataPtr);
				m_DataPtr = other.detach();
			}

			return *this;
		}

		inline BaseImpl &operator =(const BaseImpl &) = delete;

		inline explicit operator bool() const
		{
			return m_DataPtr != nullptr;
		}

		inline T **put()
		{
			traits::close(m_DataPtr);
			return &m_DataPtr;
		}

		inline T *get()
		{
			return m_DataPtr;
		}

		inline const T *get() const
		{
			return m_DataPtr;
		}

		inline void attach(T *ptr)
		{
			traits::close(m_DataPtr);
			m_DataPtr = ptr;
		}

		[[nodiscard]] inline T *detach()
		{
			return std::exchange(m_DataPtr, nullptr);
		}

		inline ~BaseImpl()
		{
			traits::close(m_DataPtr);
		}
	};

	template<typename T, class traits>
	class Base;

	template<typename T, class traits>
	class Base<T[], traits> : public BaseImpl<T, traits> {
	public:
		using BaseImpl<T, traits>::BaseImpl;

		inline static Base Alloc(std::size_t count)
		{
			return traits::alloc(count * sizeof(T));
		}

		inline T &operator [](std::size_t i)
		{
			return this->m_DataPtr[i];
		}

		inline const T &operator [](std::size_t i) const
		{
			return this->m_DataPtr[i];
		}
	};

	struct GlobalTraits {
		inline static void *alloc(std::size_t size)
		{
			return GlobalAlloc(GHND, size);
		}

		inline static void close(void *data)
		{
			void *result = GlobalFree(data);
			if (result)
			{
				LastErrorHandle(Error::Level::Log, L"Failed to free memory.");
			}
		}

		inline static void *lock(void *handle)
		{
			return GlobalLock(handle);
		}

		inline static void unlock(void *handle)
		{
			if (!GlobalUnlock(handle))
			{
				DWORD err = GetLastError();
				if (err != NO_ERROR)
				{
					ErrorHandle(HRESULT_FROM_WIN32(err), Error::Level::Log, L"Failed to unlock memory.");
				}
			}
		}
	};

public:
	template<typename T = void>
	using GlobalHandle = Base<T, GlobalTraits>;
};