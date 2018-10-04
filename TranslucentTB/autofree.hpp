#pragma once
#include "arch.h"
#include <objbase.h>
#include <type_traits>
#include <utility>
#include <WinBase.h>
#include <winerror.h>

#include "ttberror.hpp"

class AutoFree {
private:
	template<typename T, class traits>
	class BaseImpl {
	protected:
		T *m_DataPtr;

		inline BaseImpl(void *data) : m_DataPtr(static_cast<T *>(data)) { }

	public:
		inline BaseImpl(T *&&data = nullptr) : m_DataPtr(data) { }

		inline BaseImpl(BaseImpl &&other)
		{
			if (this != &other)
			{
				m_DataPtr = other.detach();
			}
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
			return m_DataPtr;
		}

		inline const T *operator ->() const
		{
			return m_DataPtr;
		}

		inline T &operator *()
		{
			return *m_DataPtr;
		}

		inline const T &operator *() const
		{
			return *m_DataPtr;
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
			return m_DataPtr[i];
		}

		inline const T &operator [](std::size_t i) const
		{
			return m_DataPtr[i];
		}
	};

	template<class traits>
	class Base<void, traits> : public BaseImpl<void, traits> {
	public:
		using BaseImpl<void, traits>::BaseImpl;
	};

	template<bool silent, Error::Level level = Error::Level::Log>
	struct LocalTraits {
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

	template<typename T>
	class GlobalLock {
	private:
		GlobalHandle<T> &m_Handle;
		using ptr_t = std::remove_extent_t<T> *;
		ptr_t m_Ptr;

	public:
		inline explicit GlobalLock(GlobalHandle<T> &handle) : m_Handle(handle)
		{
			m_Ptr = static_cast<ptr_t>(::GlobalLock(m_Handle.get()));
		}

		inline GlobalLock(const GlobalLock &other) = delete;
		inline GlobalLock &operator =(const GlobalLock &other) = delete;

		inline ptr_t get()
		{
			return m_Ptr;
		}

		inline explicit operator bool() const
		{
			return m_Ptr != nullptr;
		}

		inline ~GlobalLock()
		{
			if (!GlobalUnlock(m_Handle.get()))
			{
				DWORD err = GetLastError();
				if (err != NO_ERROR)
				{
					ErrorHandle(HRESULT_FROM_WIN32(err), Error::Level::Error, L"Failed to unlock memory.");
				}
			}
		}
	};
};