#pragma once
#include <type_traits>

namespace Util {
    template<typename T>
    concept Pointer = std::is_pointer_v<T>;

	template<typename T>
	concept FunctionPointer = Pointer<T> && std::is_function_v<std::remove_pointer_t<T>>;

    template<typename T>
	concept MemberFunctionPointer = Pointer<T> && std::is_member_function_pointer_v<T>;
}
