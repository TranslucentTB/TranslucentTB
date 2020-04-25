#pragma once
#include "arch.h"
#include <fileapi.h>
#include <filesystem>
#include <string_view>
#include <type_traits>
#include <windef.h>
#include <wil/resource.h>

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

	static void WINAPI OverlappedCallback(DWORD error, DWORD, OVERLAPPED *overlapped);

	void rearm();

public:
	FolderWatcher(const std::filesystem::path &path, bool recursive, DWORD filter, callback_t callback, void *context);

	~FolderWatcher();

	inline FolderWatcher(const FolderWatcher &) = delete;
	inline FolderWatcher &operator =(const FolderWatcher &) = delete;
};
