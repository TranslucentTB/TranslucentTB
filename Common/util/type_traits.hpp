#pragma once
#include <type_traits>

namespace Util {
	template<typename T>
	class decay_array {
		using U = std::remove_reference_t<T>;

	public:
		using type = std::conditional_t<std::is_array_v<U>, std::remove_extent_t<U>*, T>;
	};

	template<typename T>
	using decay_array_t = typename decay_array<T>::type;

	template<typename T>
	inline constexpr bool is_optional_v = false;

	template<typename T>
	inline constexpr bool is_optional_v<std::optional<T>> = true;

	template<typename T>
	struct is_optional : std::bool_constant<is_optional_v<T>> {};
}
