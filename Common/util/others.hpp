#pragma once
#include <functional>
#include <utility>

namespace Util {
	template<typename K, typename V, class Compare = std::less<V>>
	struct map_value_compare {
		constexpr bool operator()(const std::pair<K, V> &a, const std::pair<K, V> &b) const noexcept
		{
			static constexpr Compare comparer;
			return comparer(a.second, b.second);
		}
	};
}