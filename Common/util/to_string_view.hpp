#pragma once
#include <concepts>
#include <fmt/format.h>
#include <string_view>

namespace Util {
	template<std::convertible_to<std::string_view> T>
	inline std::string_view ToStringView(const T &thing)
	{
		return static_cast<std::string_view>(thing);
	}

	template<std::size_t N, class Alloc>
	inline std::string_view ToStringView(const fmt::basic_memory_buffer<char, N, Alloc> &buf)
	{
		return std::string_view(buf.data(), buf.size());
	}

	template<std::convertible_to<std::wstring_view> T>
	inline std::wstring_view ToStringView(const T &thing)
	{
		return static_cast<std::wstring_view>(thing);
	}

	template<std::size_t N, class Alloc>
	inline std::wstring_view ToStringView(const fmt::basic_memory_buffer<wchar_t, N, Alloc> &buf)
	{
		return std::wstring_view(buf.data(), buf.size());
	}

	template<typename>
	inline constexpr bool is_convertible_to_string_view_v = false;

	template<std::convertible_to<std::string_view> T>
	inline constexpr bool is_convertible_to_string_view_v<T> = true;

	template<std::size_t N, class Alloc>
	inline constexpr bool is_convertible_to_string_view_v<fmt::basic_memory_buffer<char, N, Alloc>> = true;

	template<typename>
	inline constexpr bool is_convertible_to_wstring_view_v = false;

	template<std::convertible_to<std::wstring_view> T>
	inline constexpr bool is_convertible_to_wstring_view_v<T> = true;

	template<std::size_t N, class Alloc>
	inline constexpr bool is_convertible_to_wstring_view_v<fmt::basic_memory_buffer<wchar_t, N, Alloc>> = true;
}
