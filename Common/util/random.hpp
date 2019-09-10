#pragma once
#include <algorithm>
#include <concepts>
#include <cstdint>
#include <limits>
#include <random>
#include <unordered_map>
#include <utility>

namespace Util {
	// Correct, allocation-free, on-the-fly generated seed sequence.
	template<std::uniform_random_bit_generator T>
	class seed_generator {
	private:
		T m_Rng;

	public:
		template<typename... Args>
		inline seed_generator(Args &&...args) : m_Rng(std::forward<Args>(args)...)
		{ }

		template<typename Iterator>
		inline void generate(Iterator first, Iterator last)
		{
			std::uniform_int_distribution<uint32_t> dist;
			std::generate(first, last, [this, &dist]
			{
				return dist(m_Rng);
			});
		}
	};

	// Gets a static instance of a certain engine, correctly seeded from std::random_device.
	template<class T>
	inline T &GetRandomEngine()
	{
		static thread_local T rng = []
		{
			seed_generator<std::random_device> gen;
			return T(gen);
		}();

		return rng;
	}

	// Gets a random number from an distribution of numbers, using a Mersenne Twister engine.
	template<std::integral T = int>
	inline T GetRandomNumber(T begin = std::numeric_limits<T>::min(), T end = std::numeric_limits<T>::max())
	{
		std::uniform_int_distribution<T> distribution(begin, end);
		return distribution(GetRandomEngine<std::mt19937>());
	}

	// Gets a key which does not exist in the map yet.
	template<std::integral K, typename V>
	inline K GetSecret(const std::unordered_map<K, V> &map)
	{
		K secret;
		do
		{
			secret = GetRandomNumber<K>();
		}
		while (map.contains(secret));

		return secret;
	}
}
