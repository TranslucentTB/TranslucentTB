#pragma once
#include <chrono>
#include <semaphore>

namespace Util {
	class thread_independent_mutex {
		std::binary_semaphore semaphore;

	public:
		constexpr thread_independent_mutex() noexcept /* strengthened */ : semaphore(1) { }

		void lock() noexcept /* strengthened */
		{
			semaphore.acquire();
		}

		void unlock() noexcept /* strengthened */
		{
			semaphore.release();
		}

		bool try_lock() noexcept
		{
			return semaphore.try_acquire();
		}

		template<class Rep, class Period>
		bool try_lock_for(const std::chrono::duration<Rep, Period> &time)
		{
			return semaphore.try_acquire_for(time);
		}

		template<class Clock, class Duration>
		bool try_lock_until(const std::chrono::time_point<Clock, Duration> &time)
		{
			return semaphore.try_acquire_until(time);
		}
	};
}
