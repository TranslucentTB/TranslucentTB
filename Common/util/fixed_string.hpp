#pragma once
#include <cstddef>
#include <string_view>

namespace Util {
	template<std::size_t N, typename CharT, typename Traits = std::char_traits<CharT>>
	struct basic_fixed_string {
		consteval basic_fixed_string(const CharT (&arr)[N + 1]) noexcept
		{
			for (std::size_t i = 0; i < N; ++i)
			{
				data[i] = arr[i];
			}
		}

		constexpr operator std::basic_string_view<CharT, Traits>() const noexcept
		{
			return { data, N };
		}

		CharT data[N]{};
	};

	template<std::size_t N, typename CharT>
	basic_fixed_string(const CharT (&)[N]) -> basic_fixed_string<N - 1, CharT, std::char_traits<CharT>>;

	template<std::size_t N>
	using fixed_string = basic_fixed_string<N, char>;

#ifdef __cpp_lib_char8_t
	template<std::size_t N>
	using fixed_u8string = basic_fixed_string<N, char8_t>;
#endif

	template<std::size_t N>
	using fixed_u16string = basic_fixed_string<N, char16_t>;

	template<std::size_t N>
	using fixed_u32string = basic_fixed_string<N, char32_t>;

	template<std::size_t N>
	using fixed_wstring = basic_fixed_string<N, wchar_t>;
}
