#pragma once
#include "arch.h"
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
#include <wil/resource.h>
#include <winnt.h>

#include "util/concepts.hpp"

namespace wilx {
	// Send help
	namespace impl {
		template<typename T>
		struct function_traits;

		template<typename Parent, typename Return, typename... Args>
		struct function_traits<Return(STDMETHODCALLTYPE Parent:: *)(Args...)> {
			using parent = Parent;

			template<std::size_t I>
			using arg = std::tuple_element_t<I, std::tuple<Args...>>;
		};

		template<typename Parent, typename Return, typename... Args>
		struct function_traits<Return(STDMETHODCALLTYPE Parent:: *)(Args...) noexcept> {
			using parent = Parent;

			template<std::size_t I>
			using arg = std::tuple_element_t<I, std::tuple<Args...>>;
		};

		template<typename Return, typename... Args>
		struct function_traits<Return(STDMETHODCALLTYPE *)(Args...)> {
			template<std::size_t I>
			using arg = std::tuple_element_t<I, std::tuple<Args...>>;
		};

		template<typename Return, typename... Args>
		struct function_traits<Return(STDMETHODCALLTYPE *)(Args...) noexcept> {
			template<std::size_t I>
			using arg = std::tuple_element_t<I, std::tuple<Args...>>;
		};

		template<typename T>
		using parent_t = typename function_traits<T>::parent;

		template<typename T, std::size_t I>
		using arg_t = typename function_traits<T>::template arg<I>;
	}

	template<auto close_fn, impl::arg_t<decltype(close_fn), 0> invalid_token = {}>
	requires std::is_member_function_pointer_v<decltype(close_fn)>
	using unique_com_token = wil::unique_com_token<impl::parent_t<decltype(close_fn)>, decltype(invalid_token), decltype(close_fn), close_fn, invalid_token>;

	template<auto close_fn>
	requires std::is_member_function_pointer_v<decltype(close_fn)>
	using unique_com_call = wil::unique_com_call<impl::parent_t<decltype(close_fn)>, decltype(close_fn), close_fn>;

	template<Util::function_pointer auto close_fn>
	using unique_any = wil::unique_any<impl::arg_t<decltype(close_fn), 0>, decltype(close_fn), close_fn>;

	template<Util::function_pointer auto close_fn>
	using unique_any_handle_invalid = wil::unique_any_handle_invalid<decltype(close_fn), close_fn>;

	template<Util::function_pointer auto close_fn>
	using unique_call = wil::unique_call<decltype(close_fn), close_fn>;

	template<Util::function_pointer auto delete_fn>
	using function_deleter = wil::function_deleter<decltype(delete_fn), delete_fn>;
}
