#pragma once
#include <cassert>
#include <concepts>
#include <cstddef>
#include <memory>
#include <new>

namespace Util {
	namespace impl {
		template<typename T>
		struct member_array_pointer_traits;

		template<typename T, typename Class>
		struct member_array_pointer_traits<T(Class::*)[]> {
			using parent = Class;
			using member = T;
		};

		template<auto data>
		using parent_t = typename member_array_pointer_traits<decltype(data)>::parent;

		template<auto data>
		using member_t = typename member_array_pointer_traits<decltype(data)>::member;
	}

	template<typename T>
	struct flexible_array_traits;

	template<typename derived>
	class flexible_array {
	private:
		std::size_t _size;

		static constexpr auto data() noexcept
		{
			return flexible_array_traits<derived>::data;
		}

		static constexpr derived *crtp(flexible_array *ptr) noexcept
		{
			return static_cast<derived *>(ptr);
		}

		static constexpr std::size_t allocation_size(std::size_t arrSize) noexcept
		{
			return sizeof(derived) + (sizeof(impl::member_t<data()>) * arrSize);
		}

		void *operator new(std::size_t size, std::size_t arr)
		{
			assert(sizeof(derived) == size);
			return ::operator new(allocation_size(arr));
		}

		flexible_array(const flexible_array &) = delete;
		flexible_array &operator =(const flexible_array &) = delete;

	protected:
		constexpr flexible_array() noexcept = default;

		~flexible_array()
		{
			std::destroy_n(crtp(this)->*(data()), _size);
		}

	public:
		constexpr std::size_t size() const noexcept { return _size; }

		template<typename... Args>
#ifdef __cpp_lib_concepts // MIGRATION: IDE concept support
		requires std::same_as<derived, impl::parent_t<data()>>
#endif
		static std::unique_ptr<derived> make(std::size_t size, Args &&...args)
		{
			std::unique_ptr<derived> ptr(new (size) derived(std::forward<Args>(args)...));
			ptr->_size = size;

			std::uninitialized_default_construct_n(ptr.get()->*(data()), size);
			return ptr;
		}

#ifdef __cpp_lib_destroying_delete // MIGRATION: compiler destroying delete support
		void operator delete(flexible_array *ptr, std::destroying_delete_t)
		{
			const std::size_t sizeCopy = ptr->_size;
			crtp(ptr)->~derived();

			::operator delete(ptr, allocation_size(sizeCopy));
		}
#else
		void operator delete(void *ptr) noexcept
		{
			::operator delete(ptr);
		}
#endif
	};
}
