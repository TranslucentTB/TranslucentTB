#pragma once
#include <cstddef>
#include <cstdint>

namespace Util {
#ifdef _WIN64
	namespace impl {
		static constexpr std::size_t FNV_PRIME = 0x100000001B3;
	}

	static constexpr std::size_t INITIAL_HASH_VALUE = 0xCBF29CE484222325;
#else
	namespace impl {
		static constexpr std::size_t FNV_PRIME = 0x1000193;
	}

	static constexpr std::size_t INITIAL_HASH_VALUE = 0x811C9DC5;
#endif

	constexpr void HashByte(std::size_t &h, uint8_t b)
	{
		h ^= b;
		h *= impl::FNV_PRIME;
	}
}
