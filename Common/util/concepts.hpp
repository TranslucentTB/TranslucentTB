#pragma once
#include <type_traits>

namespace Util {
	template<typename T>
	concept pointer = std::is_pointer_v<T>;

	template<typename T>
	concept function_pointer = pointer<T> && std::is_function_v<std::remove_pointer_t<T>>;

	template<typename T>
	concept member_function_pointer = pointer<T> && std::is_member_function_pointer_v<T>;
}
