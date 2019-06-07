#pragma once
#include <algorithm>
#include <limits>
#include <random>
#include <unordered_map>

namespace Util {
	namespace impl {
		// Gets a static instance of a Mersenne Twister engine. Can't be put directly in
		// GetRandomNumber because every different template instantion will get a different static variable.
		template<class T>
		inline T &GetRandomEngine()
		{
			static T rng = []
			{
				std::seed_seq::result_type seed_data[T::state_size];
				std::random_device r;

				std::generate_n(seed_data, T::state_size, std::ref(r));
				std::seed_seq seed(seed_data, seed_data + T::state_size);
				return T(seed);
			}();

			return rng;
		}
	}

	// Gets a random number from an distribution of numbers.
	template<typename T = int>
	inline T GetRandomNumber(T begin = std::numeric_limits<T>::min(), T end = std::numeric_limits<T>::max())
	{
		std::uniform_int_distribution<T> distribution(begin, end);
		return distribution(impl::GetRandomEngine<std::mt19937>());
	}

	template<typename K, typename V>
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