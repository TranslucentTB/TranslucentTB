#pragma once
#include <concepts>
#include <fmt/format.h>
#include <string_view>
#ifndef __cpp_concepts // MIGRATION: IDE concept support
# include <type_traits>
#endif

namespace Util {
#ifdef __cpp_concepts // MIGRATION: IDE concept support
	template<std::convertible_to<std::wstring_view> T>
#else
	template<typename T, std::enable_if_t<std::is_convertible_v<T, std::wstring_view>, int> = 0>
#endif
	inline std::wstring_view ToStringView(const T &thing)
	{
		return static_cast<std::wstring_view>(thing);
	}

	template<std::size_t N, class Alloc>
	inline std::wstring_view ToStringView(const fmt::basic_memory_buffer<wchar_t, N, Alloc> &buf)
	{
		return std::wstring_view(buf.data(), buf.size());
	}

#ifdef __cpp_concepts // MIGRATION: IDE concept support
	template<typename>
#else
	template<typename, class = void>
#endif
	inline constexpr bool is_convertible_to_wstring_view_v = false;

#ifdef __cpp_concepts // MIGRATION: IDE concept support
	template<std::convertible_to<std::wstring_view> T>
	inline constexpr bool is_convertible_to_wstring_view_v<T> = true;
#else
	template<typename T>
	inline constexpr bool is_convertible_to_wstring_view_v<T, std::enable_if_t<std::is_convertible_v<T, std::wstring_view>>> = true;
#endif

	template<std::size_t N, class Alloc>
	inline constexpr bool is_convertible_to_wstring_view_v<fmt::basic_memory_buffer<wchar_t, N, Alloc>> = true;
}
