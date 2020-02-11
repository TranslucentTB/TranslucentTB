#pragma once
#include <type_traits>

namespace Util {
#ifdef __cpp_concepts // MIGRATION: IDE concept support
	template<typename T>
	concept function_pointer = std::is_pointer_v<T> && std::is_function_v<std::remove_pointer_t<T>>;
#endif
}
