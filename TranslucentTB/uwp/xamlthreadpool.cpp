#include "xamlthreadpool.hpp"
#include <wil/safecast.h>

XamlThread &XamlThreadPool::GetAvailableThread(std::unique_lock<Util::thread_independent_mutex> &lock)
{
	for (auto it = m_Threads.begin(); it != m_Threads.end(); ++it)
	{
		XamlThread &thread = **it;
		lock = thread.Lock();
		if (thread.IsAvailable())
		{
			return thread;
		}
		else
		{
			lock.unlock();
		}
	}

	auto &thread = m_Threads.emplace_back(std::make_unique<XamlThread>());
	lock = thread->Lock();
	return *thread;
}

XamlThreadPool::~XamlThreadPool()
{
	std::vector<wil::unique_handle> threads;
	threads.reserve(m_Threads.size());

	for (auto it = m_Threads.begin(); it != m_Threads.end();)
	{
		threads.push_back(it->get()->Delete());
		static_cast<void>(it->release());

		it = m_Threads.erase(it);
	}

	// verify this shady cast is actually doable...
	static_assert(sizeof(decltype(threads)::value_type) == sizeof(HANDLE));

	if (WaitForMultipleObjects(wil::safe_cast<DWORD>(threads.size()), reinterpret_cast<HANDLE *>(threads.data()), true, INFINITE) == WAIT_FAILED)
	{
		LastErrorHandle(spdlog::level::warn, L"Failed to wait for thread termination");
	}

	threads.clear();
	m_App = nullptr;
}
