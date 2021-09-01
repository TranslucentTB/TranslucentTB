#pragma once
#include <fmt/format.h>

namespace Util {
	template<std::size_t n>
	using small_memory_buffer = fmt::basic_memory_buffer<char, n>;

	template<std::size_t n>
	using small_wmemory_buffer = fmt::basic_memory_buffer<wchar_t, n>;
}
