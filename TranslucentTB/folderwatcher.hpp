#pragma once
#include "arch.h"
#include <fileapi.h>
#include <filesystem>
#include <functional>
#include <synchapi.h>
#include <thread>
#include <WinBase.h>
#include <winrt/base.h>

#include "constants.hpp"
#include "ttberror.hpp"
#include "windows/window.hpp"

class FolderWatcher {
private:
	struct ChangeNotificationHandleTraits {
		using type = HANDLE;

		inline static void close(type value) noexcept
		{
			WINRT_VERIFY(FindCloseChangeNotification(value));
		}

		inline static type invalid() noexcept
		{
			return INVALID_HANDLE_VALUE;
		}
	};

	winrt::handle m_Event;
	winrt::handle_type<ChangeNotificationHandleTraits> m_ChangeHandle;
	Window m_CallbackWnd;
	std::thread m_WatcherThread;

	inline void thread_proc()
	{
		HANDLE handles[] = {
			m_ChangeHandle.get(),
			m_Event.get()
		};

		while (true)
		{
			DWORD ret = WaitForMultipleObjects(2, handles, false, INFINITE);
			if (ret != WAIT_FAILED)
			{
				if (ret == WAIT_OBJECT_0)
				{
					m_CallbackWnd.send_message(WM_FILECHANGED);
					if (!FindNextChangeNotification(m_ChangeHandle.get()))
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
	inline FolderWatcher(const std::filesystem::path &folder, DWORD filter, Window callback) : m_CallbackWnd(callback)
	{
		m_Event.attach(CreateEvent(NULL, TRUE, FALSE, NULL));
		if (!m_Event)
		{
			LastErrorHandle(Error::Level::Error, L"Failed to create synchronization event. Hot loading of config will not work.");
			return;
		}

		m_ChangeHandle.attach(FindFirstChangeNotification(folder.c_str(), false, filter));
		if (!m_ChangeHandle)
		{
			LastErrorHandle(Error::Level::Error, L"Failed to start watching for configuration changes. Hot loading of config will not work.");
			m_Event.close();
			return;
		}

		m_WatcherThread = std::thread(std::bind(&FolderWatcher::thread_proc, this));
	}

	inline ~FolderWatcher()
	{
		if (m_WatcherThread.joinable())
		{
			if (!SetEvent(m_Event.get()))
			{
				LastErrorHandle(Error::Level::Fatal, L"Failed to set synchronization event. Deadlock detected.");
			}

			m_WatcherThread.join();
		}
	}

	inline FolderWatcher(const FolderWatcher &) = delete;
	inline FolderWatcher &operator =(const FolderWatcher &) = delete;
};