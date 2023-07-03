#pragma once
#include <format>
#include <string>
#include <string_view>

namespace Util {
	template<typename char_type, class traits = std::char_traits<char_type>>
	class basic_null_terminated_string_view : public std::basic_string_view<char_type, traits> {
		using base = std::basic_string_view<char_type, traits>;

		constexpr basic_null_terminated_string_view(const typename base::const_pointer str, const typename base::size_type length) noexcept :
			base(str, length)
		{ }

	public:
		using base::base;

		constexpr void remove_suffix(const typename base::size_type) noexcept = delete;
		constexpr base substr(const typename base::size_type, typename base::size_type) const = delete;

		template<class allocator>
		constexpr basic_null_terminated_string_view(const std::basic_string<char_type, traits, allocator> &str) :
			base(str.c_str(), str.length())
		{ }

		constexpr typename base::const_pointer c_str() const noexcept
		{
			return base::data();
		}

		// only call this if you're sure it actually is terminated by a null pointer.
		static constexpr basic_null_terminated_string_view make_unsafe(const typename base::const_pointer str, const typename base::size_type length) noexcept
		{
			return { str, length };
		}
	};

	using null_terminated_string_view = basic_null_terminated_string_view<char>;
	using null_terminated_u8string_view = basic_null_terminated_string_view<char8_t>;
	using null_terminated_u16string_view = basic_null_terminated_string_view<char16_t>;
	using null_terminated_u32string_view = basic_null_terminated_string_view<char32_t>;
	using null_terminated_wstring_view = basic_null_terminated_string_view<wchar_t>;
}

template<typename char_type, class traits>
struct std::formatter<Util::basic_null_terminated_string_view<char_type, traits>, char_type> : std::formatter<std::basic_string_view<char_type, traits>, char_type>
{};
