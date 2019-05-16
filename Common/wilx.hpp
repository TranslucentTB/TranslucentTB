#pragma once
#include <cstddef>
#include <tuple>
#include <wil/resource.h>

namespace wilx {
	// Send help
	namespace impl {
		template<typename T>
		struct member_function_traits;

		template<typename Parent, typename Return, typename... Args>
		struct member_function_traits<Return(STDMETHODCALLTYPE Parent:: *)(Args...)> {
			using parent = Parent;

			template<std::size_t i>
			using arg = std::tuple_element_t<i, std::tuple<Args...>>;
		};

		template<typename T>
		using member_parent_t = typename member_function_traits<T>::parent;

		template<typename T, std::size_t i>
		using member_arg_t = typename member_function_traits<T>::arg<i>;
	}

	template<auto close_fn, impl::member_arg_t<decltype(close_fn), 0> invalid_token = impl::member_arg_t<decltype(close_fn), 0>()>
	using unique_com_token = wil::unique_com_token<impl::member_parent_t<decltype(close_fn)>, impl::member_arg_t<decltype(close_fn), 0>, decltype(close_fn), close_fn, invalid_token>;

	template<auto close_fn>
	using unique_com_call = wil::unique_com_call<impl::member_parent_t<decltype(close_fn)>, decltype(close_fn), close_fn>;
}