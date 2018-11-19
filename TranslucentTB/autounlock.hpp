#pragma once
#include <type_traits>

#include "autofree.hpp"

template<typename T, class traits, typename = std::enable_if_t<traits::needs_lock>>
class AutoUnlock : private AutoFree {
private:
	BaseImpl<T, traits> &m_smartHandle;
	T *m_originalHandle;

public:
	inline explicit AutoUnlock(BaseImpl<T, traits> &handle) : m_smartHandle(handle), m_originalHandle(handle.detach())
	{
		handle.attach(static_cast<T *>(traits::lock(m_originalHandle)));
	}

	inline AutoUnlock(const AutoUnlock &) = delete;
	inline AutoUnlock &operator =(const AutoUnlock &) = delete;

	inline ~AutoUnlock()
	{
		traits::unlock(m_originalHandle);
		(void)m_smartHandle.detach();
		m_smartHandle.attach(m_originalHandle);
	}
};