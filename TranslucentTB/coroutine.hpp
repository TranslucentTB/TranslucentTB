// University of Illinois/NCSA
// Open Source License
// 
// Copyright (c) 2009-2017 by the contributors listed in CREDITS.TXT
// 
// All rights reserved.
// 
// Developed by:
// 
//     LLVM Team
// 
//     University of Illinois at Urbana-Champaign
// 
//     http://llvm.org
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal with
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is furnished to do
// so, subject to the following conditions:
// 
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimers.
// 
//     * Redistributions in binary form must reproduce the above copyright notice,
//       this list of conditions and the following disclaimers in the
//       documentation and/or other materials provided with the distribution.
// 
//     * Neither the names of the LLVM Team, University of Illinois at
//       Urbana-Champaign, nor the names of its contributors may be used to
//       endorse or promote products derived from this Software without specific
//       prior written permission.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE
// SOFTWARE.

#pragma once

#include <new>
#include <type_traits>
#include <functional>
#include <memory>
#include <cstddef>
#include <crtdbg.h>

#define _LIBCPP_ASSERT _ASSERT_EXPR
#define _LIBCPP_CONSTEXPR constexpr
#define _LIBCPP_CONSTEXPR_AFTER_CXX17 constexpr
#define _LIBCPP_INLINE_VISIBILITY inline
#define _LIBCPP_TEMPLATE_VIS
#define _LIBCPP_TYPE_VIS
#define _NOEXCEPT noexcept
#define __void_t void_t
#define _VSTD std
#define _VSTD_CORO std::experimental

namespace std::experimental {

template <class _Tp, class = void>
struct __coroutine_traits_sfinae {};

template <class _Tp>
struct __coroutine_traits_sfinae<
    _Tp, __void_t<typename _Tp::promise_type>>
{
  using promise_type = typename _Tp::promise_type;
};

template <typename _Ret, typename... _Args>
struct _LIBCPP_TEMPLATE_VIS coroutine_traits
    : public __coroutine_traits_sfinae<_Ret>
{
};

template <typename _Promise = void>
class _LIBCPP_TEMPLATE_VIS coroutine_handle;

template <>
class _LIBCPP_TEMPLATE_VIS coroutine_handle<void> {
public:
    _LIBCPP_INLINE_VISIBILITY
    _LIBCPP_CONSTEXPR coroutine_handle() _NOEXCEPT : __handle_(nullptr) {}

    _LIBCPP_INLINE_VISIBILITY
    _LIBCPP_CONSTEXPR coroutine_handle(nullptr_t) _NOEXCEPT : __handle_(nullptr) {}

    _LIBCPP_INLINE_VISIBILITY
    coroutine_handle& operator=(nullptr_t) _NOEXCEPT {
        __handle_ = nullptr;
        return *this;
    }

    _LIBCPP_INLINE_VISIBILITY
    _LIBCPP_CONSTEXPR void* address() const _NOEXCEPT { return __handle_; }

    _LIBCPP_INLINE_VISIBILITY
    _LIBCPP_CONSTEXPR explicit operator bool() const _NOEXCEPT { return __handle_; }

    _LIBCPP_INLINE_VISIBILITY
    void operator()() { resume(); }

    _LIBCPP_INLINE_VISIBILITY
    void resume() {
      _LIBCPP_ASSERT(__is_suspended(),
                     "resume() can only be called on suspended coroutines");
      _LIBCPP_ASSERT(!done(),
                "resume() has undefined behavior when the coroutine is done");
      __builtin_coro_resume(__handle_);
    }

    _LIBCPP_INLINE_VISIBILITY
    void destroy() {
      _LIBCPP_ASSERT(__is_suspended(),
                     "destroy() can only be called on suspended coroutines");
      __builtin_coro_destroy(__handle_);
    }

    _LIBCPP_INLINE_VISIBILITY
    bool done() const {
      _LIBCPP_ASSERT(__is_suspended(),
                     "done() can only be called on suspended coroutines");
      return __builtin_coro_done(__handle_);
    }

public:
    _LIBCPP_INLINE_VISIBILITY
    static coroutine_handle from_address(void* __addr) _NOEXCEPT {
        coroutine_handle __tmp;
        __tmp.__handle_ = __addr;
        return __tmp;
    }

    // FIXME: Should from_address(nullptr) be allowed?
    _LIBCPP_INLINE_VISIBILITY
    static coroutine_handle from_address(nullptr_t) _NOEXCEPT {
      return coroutine_handle(nullptr);
    }

    template <class _Tp, bool _CallIsValid = false>
    static coroutine_handle from_address(_Tp*) {
      static_assert(_CallIsValid,
       "coroutine_handle<void>::from_address cannot be called with "
        "non-void pointers");
    }

private:
  bool __is_suspended() const _NOEXCEPT  {
    // FIXME actually implement a check for if the coro is suspended.
    return __handle_;
  }

  template <class _PromiseT> friend class coroutine_handle;
  void* __handle_;
};

// 18.11.2.7 comparison operators:
_LIBCPP_INLINE_VISIBILITY
bool operator==(coroutine_handle<> __x, coroutine_handle<> __y) _NOEXCEPT {
    return __x.address() == __y.address();
}
_LIBCPP_INLINE_VISIBILITY
bool operator!=(coroutine_handle<> __x, coroutine_handle<> __y) _NOEXCEPT {
    return !(__x == __y);
}
_LIBCPP_INLINE_VISIBILITY
bool operator<(coroutine_handle<> __x, coroutine_handle<> __y) _NOEXCEPT {
    return less<void*>()(__x.address(), __y.address());
}
_LIBCPP_INLINE_VISIBILITY
bool operator>(coroutine_handle<> __x, coroutine_handle<> __y) _NOEXCEPT {
    return __y < __x;
}
_LIBCPP_INLINE_VISIBILITY
bool operator<=(coroutine_handle<> __x, coroutine_handle<> __y) _NOEXCEPT {
    return !(__x > __y);
}
_LIBCPP_INLINE_VISIBILITY
bool operator>=(coroutine_handle<> __x, coroutine_handle<> __y) _NOEXCEPT {
    return !(__x < __y);
}

template <typename _Promise>
class _LIBCPP_TEMPLATE_VIS coroutine_handle : public coroutine_handle<> {
    using _Base = coroutine_handle<>;
public:
#ifndef _LIBCPP_CXX03_LANG
    // 18.11.2.1 construct/reset
    using coroutine_handle<>::coroutine_handle;
#else
    _LIBCPP_INLINE_VISIBILITY coroutine_handle() _NOEXCEPT : _Base() {}
    _LIBCPP_INLINE_VISIBILITY coroutine_handle(nullptr_t) _NOEXCEPT : _Base(nullptr) {}
#endif
    _LIBCPP_INLINE_VISIBILITY
    coroutine_handle& operator=(nullptr_t) _NOEXCEPT {
        _Base::operator=(nullptr);
        return *this;
    }

    _LIBCPP_INLINE_VISIBILITY
    _Promise& promise() const {
        return *static_cast<_Promise*>(
            __builtin_coro_promise(this->__handle_, __alignof(_Promise), false));
    }

public:
    _LIBCPP_INLINE_VISIBILITY
    static coroutine_handle from_address(void* __addr) _NOEXCEPT {
        coroutine_handle __tmp;
        __tmp.__handle_ = __addr;
        return __tmp;
    }

    // NOTE: this overload isn't required by the standard but is needed so
    // the deleted _Promise* overload doesn't make from_address(nullptr)
    // ambiguous.
    // FIXME: should from_address work with nullptr?
    _LIBCPP_INLINE_VISIBILITY
    static coroutine_handle from_address(nullptr_t) _NOEXCEPT {
      return coroutine_handle(nullptr);
    }

    template <class _Tp, bool _CallIsValid = false>
    static coroutine_handle from_address(_Tp*) {
      static_assert(_CallIsValid,
       "coroutine_handle<promise_type>::from_address cannot be called with "
        "non-void pointers");
    }

    template <bool _CallIsValid = false>
    static coroutine_handle from_address(_Promise*) {
      static_assert(_CallIsValid,
       "coroutine_handle<promise_type>::from_address cannot be used with "
        "pointers to the coroutine's promise type; use 'from_promise' instead");
    }

    _LIBCPP_INLINE_VISIBILITY
    static coroutine_handle from_promise(_Promise& __promise) _NOEXCEPT {
        typedef typename remove_cv<_Promise>::type _RawPromise;
        coroutine_handle __tmp;
        __tmp.__handle_ = __builtin_coro_promise(
            _VSTD::addressof(const_cast<_RawPromise&>(__promise)),
             __alignof(_Promise), true);
        return __tmp;
    }
};

#if __has_builtin(__builtin_coro_noop)
struct noop_coroutine_promise {};

template <>
class _LIBCPP_TEMPLATE_VIS coroutine_handle<noop_coroutine_promise>
    : public coroutine_handle<> {
  using _Base = coroutine_handle<>;
  using _Promise = noop_coroutine_promise;
public:

  _LIBCPP_INLINE_VISIBILITY
  _Promise& promise() const {
    return *static_cast<_Promise*>(
      __builtin_coro_promise(this->__handle_, __alignof(_Promise), false));
  }

  _LIBCPP_CONSTEXPR explicit operator bool() const _NOEXCEPT { return true; }
  _LIBCPP_CONSTEXPR bool done() const _NOEXCEPT { return false; }

  _LIBCPP_CONSTEXPR_AFTER_CXX17 void operator()() const _NOEXCEPT {}
  _LIBCPP_CONSTEXPR_AFTER_CXX17 void resume() const _NOEXCEPT {}
  _LIBCPP_CONSTEXPR_AFTER_CXX17 void destroy() const _NOEXCEPT {}

private:
  _LIBCPP_INLINE_VISIBILITY
  friend coroutine_handle<noop_coroutine_promise> noop_coroutine() _NOEXCEPT;

  _LIBCPP_INLINE_VISIBILITY coroutine_handle() _NOEXCEPT {
    this->__handle_ = __builtin_coro_noop();
  }
};

using noop_coroutine_handle = coroutine_handle<noop_coroutine_promise>;

_LIBCPP_INLINE_VISIBILITY
noop_coroutine_handle noop_coroutine() _NOEXCEPT {
  return noop_coroutine_handle();
}
#endif // __has_builtin(__builtin_coro_noop)

struct _LIBCPP_TYPE_VIS suspend_never {
  _LIBCPP_INLINE_VISIBILITY
  bool await_ready() const _NOEXCEPT { return true; }
  _LIBCPP_INLINE_VISIBILITY
  void await_suspend(coroutine_handle<>) const _NOEXCEPT {}
  _LIBCPP_INLINE_VISIBILITY
  void await_resume() const _NOEXCEPT {}
};

struct _LIBCPP_TYPE_VIS suspend_always {
  _LIBCPP_INLINE_VISIBILITY
  bool await_ready() const _NOEXCEPT { return false; }
  _LIBCPP_INLINE_VISIBILITY
  void await_suspend(coroutine_handle<>) const _NOEXCEPT {}
  _LIBCPP_INLINE_VISIBILITY
  void await_resume() const _NOEXCEPT {}
};

}

namespace std {

template <class _Tp>
struct hash<_VSTD_CORO::coroutine_handle<_Tp> > {
    using __arg_type = _VSTD_CORO::coroutine_handle<_Tp>;
    _LIBCPP_INLINE_VISIBILITY
    size_t operator()(__arg_type const& __v) const _NOEXCEPT
    {return hash<void*>()(__v.address());}
};

}