#include "arch.h"
#include <functional>
#include <memory>
#include <string>
#include <synchapi.h>
#include <thread>
#include <WinBase.h>

#include "changenotificationhandle.hpp"
#include "common.hpp"
#include "ttberror.hpp"

class FolderWatcher {
private:
	using callback_t = std::function<void()>;

	winrt::handle m_event;
	change_notification_handle m_changeHandle;
	callback_t m_callback;
	std::thread m_watcherThread;

	inline void thread_proc()
	{
		HANDLE handles[] = {
			m_changeHandle.get(),
			m_event.get()
		};

		while (true)
		{
			DWORD ret = WaitForMultipleObjects(2, handles, false, INFINITE);
			if (ret != WAIT_FAILED)
			{
				if (ret == WAIT_OBJECT_0)
				{
					m_callback();
					if (!FindNextChangeNotification(m_changeHandle.get()))
					{
						LastErrorHandle(Error::Level::Log, L"Failed to resume folder watch.");
					}
				}
				else if (ret == WAIT_OBJECT_0 + 1)
				{
					return; // We are destroying
				}
			}
			else
			{
				LastErrorHandle(Error::Level::Log, L"Failed to wait for folder or event change.");
			}
		}
	}

public:
	inline FolderWatcher(const std::wstring &folder, DWORD filter, const callback_t &callback) : m_callback(callback)
	{
		m_event = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (!m_event)
		{
			LastErrorHandle(Error::Level::Log, L"Failed to create synchronization event.");
			return;
		}

		m_changeHandle = FindFirstChangeNotification(folder.c_str(), false, filter);
		if (!m_changeHandle)
		{
			LastErrorHandle(Error::Level::Log, L"Failed to start watching for changes.");
			return;
		}

		m_watcherThread = std::thread(std::bind(&FolderWatcher::thread_proc, this));
	}

	inline ~FolderWatcher()
	{
		if (m_watcherThread.joinable())
		{
			if (!SetEvent(m_event.get()))
			{
				LastErrorHandle(Error::Level::Fatal, L"Failed to set synchronization event. Deadlock detected.");
			}

			m_watcherThread.join();
		}
	}

	inline FolderWatcher(const FolderWatcher &) = delete;
	inline FolderWatcher &operator =(const FolderWatcher &) = delete;
};