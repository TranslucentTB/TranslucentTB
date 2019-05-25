#pragma once
#include <algorithm>
#include <functional>
#include <unordered_map>
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

	template<typename K, typename V>
	inline V FindOrDefault(const std::unordered_map<K, V> &map, const K &key)
	{
		if (const auto iter = map.find(key); iter != map.end())
		{
			return iter->second;
		}
		else
		{
			return { };
		}
	}

	template<typename K, typename V>
	typename std::unordered_map<K, V>::const_iterator FindValue(const std::unordered_map<K, V> &map, const V &value)
	{
		return std::find_if(map.begin(), map.end(), [&value](const std::pair<K, V> &pair)
		{
			return pair.second == value;
		});
	}
}