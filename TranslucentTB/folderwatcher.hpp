#include "arch.h"
#include <functional>
#include <string>
#include <synchapi.h>
#include <thread>
#include <WinBase.h>

#include "changenotificationhandle.hpp"
#include "common.hpp"
#include "ttberror.hpp"
#include "window.hpp"

class FolderWatcher {
private:
	winrt::handle m_event;
	change_notification_handle m_changeHandle;
	Window m_callbackWnd;
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
					m_callbackWnd.send_message(FILE_CHANGED);
					if (!FindNextChangeNotification(m_changeHandle.get()))
					{
						LastErrorHandle(Error::Level::Log, L"Failed to resume folder watch. Hot loading of config will not work.");
						return;
					}
				}
				else if (ret == WAIT_OBJECT_0 + 1)
				{
					return; // We are destroying
				}
			}
			else
			{
				LastErrorHandle(Error::Level::Error, L"Failed to wait for folder or event change. Hot loading of config will not work.");
				return;
			}
		}
	}

public:
	inline FolderWatcher(const std::wstring &folder, DWORD filter, const Window &callback) : m_callbackWnd(callback)
	{
		m_event.attach(CreateEvent(NULL, TRUE, FALSE, NULL));
		if (!m_event)
		{
			LastErrorHandle(Error::Level::Error, L"Failed to create synchronization event. Hot loading of config will not work.");
			return;
		}

		m_changeHandle.attach(FindFirstChangeNotification(folder.c_str(), false, filter));
		if (!m_changeHandle)
		{
			LastErrorHandle(Error::Level::Error, L"Failed to start watching for configuration changes. Hot loading of config will not work.");
			m_event.close();
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