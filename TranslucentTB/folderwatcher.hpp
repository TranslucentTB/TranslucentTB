#pragma once
#include "arch.h"
#include <fileapi.h>
#include <filesystem>
#include <memory>
#include <string_view>
#include <winbase.h>
#include <windef.h>
#include <wil/filesystem.h>
#include <wil/resource.h>

#include "../ProgramLog/error/win32.hpp"

class FolderWatcher {
	static constexpr std::size_t BUFFER_SIZE = 4096;

	using callback_t = void(*)(void *, DWORD, std::wstring_view);

	bool m_Recursive;
	DWORD m_Filter;

	wil::unique_hfile m_FolderHandle;
	OVERLAPPED m_Overlapped;
	std::unique_ptr<char[]> m_Buffer;

	callback_t m_Callback;
	void *m_Context;

	inline static void callback(DWORD error, DWORD, OVERLAPPED *overlapped)
	{
		const auto that = static_cast<FolderWatcher *>(overlapped->hEvent);
		switch (error)
		{
		case ERROR_SUCCESS:
			for (const auto &fileEntry : wil::create_next_entry_offset_iterator(reinterpret_cast<FILE_NOTIFY_INFORMATION *>(that->m_Buffer.get())))
			{
				that->m_Callback(that->m_Context, fileEntry.Action, { fileEntry.FileName, fileEntry.FileNameLength / 2 });
			}

			that->rearm();
			break;

		case ERROR_NOTIFY_ENUM_DIR:
			that->m_Callback(that->m_Context, 0, { });
			that->rearm();
			break;

		default:
			LastErrorHandle(spdlog::level::warn, L"Error occured while watching directory");
			[[fallthrough]];
		case ERROR_OPERATION_ABORTED:
			break;
		}
	}

	inline void rearm()
	{
		if (!ReadDirectoryChangesW(m_FolderHandle.get(), m_Buffer.get(), BUFFER_SIZE, m_Recursive, m_Filter, nullptr, &m_Overlapped, callback))
		{
			LastErrorHandle(spdlog::level::warn, L"Failed to arm directory watcher");
		}
	}

public:
	inline FolderWatcher(const std::filesystem::path &path, bool recursive, DWORD filter, callback_t callback, void *context) :
		m_Recursive(recursive),
		m_Filter(filter),
		m_Overlapped {
			.hEvent = this // not used for ReadDirectoryChanges so we can stash the this pointer in it.
		},
		m_Buffer(std::make_unique<char[]>(BUFFER_SIZE)),
		m_Callback(callback),
		m_Context(context)
	{
		m_FolderHandle.reset(CreateFile(
			path.c_str(), FILE_LIST_DIRECTORY,
			FILE_SHARE_READ | FILE_SHARE_DELETE | FILE_SHARE_WRITE, nullptr,
			OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr
		));

		if (m_FolderHandle)
		{
			rearm();
		}
		else
		{
			LastErrorHandle(spdlog::level::warn, L"Failed to open folder handle");
		}
	}

	inline ~FolderWatcher()
	{
		if (m_FolderHandle)
		{
			if (!CancelIo(m_FolderHandle.get()))
			{
				LastErrorHandle(spdlog::level::warn, L"Failed to cancel asynchronous IO");
			}
		}
	}

	inline FolderWatcher(const FolderWatcher &) = delete;
	inline FolderWatcher &operator =(const FolderWatcher &) = delete;
};
