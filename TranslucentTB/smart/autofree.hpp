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
	class Base : public BaseImpl<T, traits> {
	public:
		using BaseImpl<T, traits>::BaseImpl;

		inline static Base Alloc()
		{
			return traits::alloc(sizeof(T));
		}

		inline T *operator ->()
		{
			return this->m_DataPtr;
		}

		inline const T *operator ->() const
		{
			return this->m_DataPtr;
		}

		inline T &operator *()
		{
			return *this->m_DataPtr;
		}

		inline const T &operator *() const
		{
			return *this->m_DataPtr;
		}
	};

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

	template<class traits>
	class Base<void, traits> : public BaseImpl<void, traits> {
	public:
		using BaseImpl<void, traits>::BaseImpl;
	};

	template<bool silent, Error::Level level = Error::Level::Log>
	struct LocalTraits {
		static constexpr bool needs_lock = false;

		inline static void *alloc(std::size_t size)
		{
			return LocalAlloc(LPTR, size);
		}

		inline static void close(void *data)
		{
			void *result = LocalFree(data);
			if (result && !silent)
			{
				LastErrorHandle(level, L"Failed to free memory.");
			}
		}
	};

	struct CoTaskMemTraits {
		static constexpr bool needs_lock = false;

		inline static void *alloc(std::size_t size)
		{
			return CoTaskMemAlloc(size);
		}

		inline static void close(void *data)
		{
			CoTaskMemFree(data);
		}
	};

	template<unsigned int flags>
	struct GlobalTraits {
		static constexpr bool needs_lock = static_cast<bool>(flags & GMEM_MOVEABLE);

		inline static void *alloc(std::size_t size)
		{
			return GlobalAlloc(flags, size);
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
	using CoTaskMem = Base<T, CoTaskMemTraits>;

	template<typename T = void>
	using Local = Base<T, LocalTraits<false>>;

	template<typename T = void>
	using DebugLocal = Base<T, LocalTraits<false, Error::Level::Debug>>;

	// Only use this if you don't want to log it for a very valid reason.
	template<typename T = void>
	using SilentLocal = Base<T, LocalTraits<true>>;

	template<typename T = void>
	using Global = Base<T, GlobalTraits<GPTR>>;

	template<typename T = void>
	using GlobalHandle = Base<T, GlobalTraits<GHND>>;
};