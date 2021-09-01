#pragma once
#include <concepts>
#include <fmt/format.h>
#include <string_view>
#include <type_traits>

namespace Util {
	template<typename T>
	requires std::convertible_to<const T &, std::wstring_view>
	inline std::wstring_view ToStringView(const T &thing) noexcept(std::is_nothrow_convertible_v<const T &, std::wstring_view>)
	{
		return static_cast<std::wstring_view>(thing);
	}

	template<std::size_t N, class Alloc>
	inline std::wstring_view ToStringView(const fmt::basic_memory_buffer<wchar_t, N, Alloc> &buf) noexcept
	{
		return std::wstring_view(buf.data(), buf.size());
	}

	template<typename>
	inline constexpr bool is_convertible_to_wstring_view_v = false;

	template<typename T>
	requires std::convertible_to<const T &, std::wstring_view>
	inline constexpr bool is_convertible_to_wstring_view_v<T> = true;

	template<std::size_t N, class Alloc>
	inline constexpr bool is_convertible_to_wstring_view_v<fmt::basic_memory_buffer<wchar_t, N, Alloc>> = true;
}
