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

	for (auto &xamlThread : m_Threads)
	{
		threads.push_back(xamlThread->Delete());
		static_cast<void>(xamlThread.release());
	}

	if (WaitForMultipleObjects(wil::safe_cast<DWORD>(threads.size()), threads.data()->addressof(), true, INFINITE) == WAIT_FAILED)
	{
		LastErrorHandle(spdlog::level::warn, L"Failed to wait for thread termination");
	}
}
