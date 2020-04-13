#pragma once
#include "arch.h"
#include <fileapi.h>
#include <filesystem>
#include <memory>
#include <string_view>
#include <type_traits>
#include <winbase.h>
#include <windef.h>
#include <wil/filesystem.h>
#include <wil/resource.h>

#include "../ProgramLog/error/win32.hpp"

class FolderWatcher {
	using callback_t = std::add_pointer_t<void(void *, DWORD, std::wstring_view)>;

	// this needs to be first for our little casting trick to work
	OVERLAPPED m_Overlapped;

	bool m_Recursive;
	DWORD m_Filter;

	wil::unique_hfile m_FolderHandle;
	callback_t m_Callback;

	wil::unique_virtualalloc_ptr<char[]> m_Buffer;
	DWORD m_BufferSize;

	inline static void WINAPI callback(DWORD error, DWORD, OVERLAPPED *overlapped)
	{
		// getting parent pointer by casting first child pointer needs standard layout.
		static_assert(std::is_standard_layout_v<FolderWatcher>);

		const auto that = reinterpret_cast<FolderWatcher *>(overlapped);
		switch (error)
		{
		case ERROR_SUCCESS:
			for (const auto &fileEntry : wil::create_next_entry_offset_iterator(reinterpret_cast<FILE_NOTIFY_INFORMATION *>(that->m_Buffer.get())))
			{
				that->m_Callback(overlapped->hEvent, fileEntry.Action, { fileEntry.FileName, fileEntry.FileNameLength / 2 });
			}

			that->rearm();
			break;

		case ERROR_NOTIFY_ENUM_DIR:
			that->m_Callback(overlapped->hEvent, 0, { });
			that->rearm();
			break;

		case ERROR_OPERATION_ABORTED:
			break;

		default:
			that->m_FolderHandle.reset();
			that->m_Buffer.reset();
			LastErrorHandle(spdlog::level::warn, L"Error occured while watching directory");
			break;
		}
	}

	inline void rearm()
	{
		if (!ReadDirectoryChangesW(m_FolderHandle.get(), m_Buffer.get(), m_BufferSize, m_Recursive, m_Filter, nullptr, &m_Overlapped, callback))
		{
			m_FolderHandle.reset();
			m_Buffer.reset();
			LastErrorHandle(spdlog::level::warn, L"Failed to arm directory watcher");
		}
	}

public:
	inline FolderWatcher(const std::filesystem::path &path, bool recursive, DWORD filter, callback_t callback, void *context) :
		m_Overlapped {
			.hEvent = context // not used for ReadDirectoryChanges so we can stash the user context pointer in it.
		},
		m_Recursive(recursive),
		m_Filter(filter),
		m_Callback(callback),
		m_BufferSize()
	{
		m_FolderHandle.reset(CreateFile(
			path.c_str(), FILE_LIST_DIRECTORY,
			FILE_SHARE_READ | FILE_SHARE_DELETE | FILE_SHARE_WRITE, nullptr,
			OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr
		));

		if (m_FolderHandle)
		{
			SYSTEM_INFO info;
			GetSystemInfo(&info);
			m_BufferSize = std::max(info.dwPageSize, info.dwAllocationGranularity);

			m_Buffer.reset(reinterpret_cast<char*>(VirtualAlloc(nullptr, m_BufferSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE)));
			if (m_Buffer)
			{
				rearm();
			}
			else
			{
				m_FolderHandle.reset();
				LastErrorHandle(spdlog::level::warn, L"Failed to allocate overlapped IO buffer");
			}
		}
		else
		{
			LastErrorHandle(spdlog::level::warn, L"Failed to open folder handle");
		}
	}

	inline ~FolderWatcher()
	{
		if (m_FolderHandle && m_Buffer)
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
