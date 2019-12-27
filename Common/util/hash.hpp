#pragma once
#include <cstddef>

namespace Util {
#ifdef _WIN64
	constexpr void HashCombine(std::size_t &h, std::size_t k) noexcept
	{
		constexpr std::size_t m = 0xC6A4A7935BD1E995;
		constexpr int r = 47;

		k *= m;
		k ^= k >> r;
		k *= m;

		h ^= k;
		h *= m;

		// Completely arbitrary number, to prevent 0's
		// from hashing to 0.
		h += 0xE6546B64;
	}
#else
	namespace impl {
		constexpr std::size_t rotl(std::size_t original, uint8_t bits) noexcept
		{
			return (original << bits) | (original >> (32 - bits));
		}
	}

	constexpr void HashCombine(std::size_t &h, std::size_t k) noexcept
	{
		constexpr std::size_t c1 = 0xCC9E2D51;
		constexpr std::size_t c2 = 0x1B873593;

		k *= c1;
		k = impl::rotl(k, 15);
		k *= c2;

		h ^= k;
		h = impl::rotl(h, 13);
		h = h * 5 + 0xE6546B64;
	}
#endif
}
