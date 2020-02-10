#pragma once
#include <cstddef>

namespace Util {
	template<typename T>
	struct flexible_array {
		inline void *operator new(std::size_t size, std::size_t arr)
		{
			return new char[size + (sizeof(T) * arr)];
		}

		inline void operator delete(void *ptr) noexcept
		{
			delete[] ptr;
		}
	};
}
