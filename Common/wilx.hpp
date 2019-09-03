#ifndef _WILX_
#define _WILX_

#include "arch.h"
#include <cstddef>
#include <tuple>
#include <utility>
#include <wil/resource.h>
#include <winnt.h>

#include "util/concepts.hpp"

namespace wilx {
	// Send help
	namespace impl {
		template</*Util::Pointer*/ typename T>
		struct function_traits;

		template<typename Parent, typename Return, typename... Args>
		struct function_traits<Return(STDMETHODCALLTYPE Parent:: *)(Args...)> {
			using parent = Parent;

			template<std::size_t I>
			using arg = std::tuple_element_t<I, std::tuple<Args...>>;
		};

		template<typename Return, typename... Args>
		struct function_traits<Return(*)(Args...)> {
			template<std::size_t I>
			using arg = std::tuple_element_t<I, std::tuple<Args...>>;
		};

		template</*Util::MemberFunctionPointer*/ typename T>
		using parent_t = typename function_traits<T>::parent;

		template</*Util::Pointer*/ typename T, std::size_t I>
		using arg_t = typename function_traits<T>::template arg<I>;
	}

	template</*Util::MemberFunctionPointer*/ auto close_fn, impl::arg_t<decltype(close_fn), 0> invalid_token = impl::arg_t<decltype(close_fn), 0>()>
	using unique_com_token = wil::unique_com_token<impl::parent_t<decltype(close_fn)>, impl::arg_t<decltype(close_fn), 0>, decltype(close_fn), close_fn, invalid_token>;

	template</*Util::MemberFunctionPointer*/ auto close_fn>
	using unique_com_call = wil::unique_com_call<impl::parent_t<decltype(close_fn)>, decltype(close_fn), close_fn>;

	template</*Util::FunctionPointer*/ auto close_fn>
	using unique_any = wil::unique_any<impl::arg_t<decltype(close_fn), 0>, decltype(close_fn), close_fn>;

	template</*Util::FunctionPointer*/ auto delete_fn>
	using function_deleter = wil::function_deleter<decltype(delete_fn), delete_fn>;
}

#endif // _WILX_

#if defined(__IAppVisibility_INTERFACE_DEFINED__) && !defined(CINTERFACE) && !defined(_WILX_IAV_)
#define _WILX_IAV_

namespace wilx {
	using unique_app_visibility_token = unique_com_token<&IAppVisibility::Unadvise>;
}

#endif
