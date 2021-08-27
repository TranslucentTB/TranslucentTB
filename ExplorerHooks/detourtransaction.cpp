#include "detourtransaction.hpp"
#include "arch.h"
#include <windef.h>
#include <winbase.h>
#include <handleapi.h>
#include <detours/detours.h>
#include <new>
#include <processthreadsapi.h>
#include <TlHelp32.h>
#include <utility>
#include <wil/result.h>

DetourTransaction::unique_hheap_failfast DetourTransaction::s_Heap;

void DetourTransaction::HeapDestroyFailFast(HANDLE hHeap) noexcept
{
	FAIL_FAST_IF_WIN32_BOOL_FALSE(HeapDestroy(hHeap));
}

void DetourTransaction::node_deleter::operator()(node *ptr) const noexcept
{
	ptr->~node();
	FAIL_FAST_IF_WIN32_BOOL_FALSE(HeapFree(s_Heap.get(), 0, ptr));
}

void DetourTransaction::attach_internal(void **function, void *detour) noexcept
{
	FAIL_FAST_IF_WIN32_ERROR(DetourAttach(function, detour));
}

void DetourTransaction::detach_internal(void **function, void *detour) noexcept
{
	FAIL_FAST_IF_WIN32_ERROR(DetourDetach(function, detour));
}

void DetourTransaction::update_thread(unique_handle_failfast hThread) noexcept
{
	FAIL_FAST_IF_WIN32_ERROR(DetourUpdateThread(hThread.get()));

	const auto mem = HeapAlloc(s_Heap.get(), 0, sizeof(node));
	FAIL_FAST_LAST_ERROR_IF_NULL(mem);

	m_Head.reset(new (mem) node
	{
		.thread = std::move(hThread),
		.next = std::move(m_Head)
	});
}

DetourTransaction::DetourTransaction() noexcept
{
	if (!s_Heap)
	{
		s_Heap.reset(HeapCreate(HEAP_NO_SERIALIZE, 0, 0));
		FAIL_FAST_LAST_ERROR_IF_NULL(s_Heap);
	}

	FAIL_FAST_IF_WIN32_ERROR(DetourTransactionBegin());
}

void DetourTransaction::update_all_threads() noexcept
{
	const DWORD pid = GetCurrentProcessId();
	const unique_handle_failfast snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, pid));
	FAIL_FAST_LAST_ERROR_IF_NULL(snapshot);

	THREADENTRY32 thread = { sizeof(thread) };
	FAIL_FAST_IF_WIN32_BOOL_FALSE(Thread32First(snapshot.get(), &thread));

	const DWORD tid = GetCurrentThreadId();
	do
	{
		if (thread.th32OwnerProcessID == pid && thread.th32ThreadID != tid)
		{
			unique_handle_failfast threadHandle(OpenThread(THREAD_QUERY_INFORMATION | THREAD_GET_CONTEXT | THREAD_SET_CONTEXT | THREAD_SUSPEND_RESUME, false, thread.th32ThreadID));
			FAIL_FAST_LAST_ERROR_IF_NULL(threadHandle);

			update_thread(std::move(threadHandle));
		}
	}
	while (Thread32Next(snapshot.get(), &thread));
}

void DetourTransaction::commit() noexcept
{
	FAIL_FAST_IF_WIN32_ERROR(DetourTransactionCommit());
	m_IsTransacting = false;
}

DetourTransaction::~DetourTransaction() noexcept
{
	if (m_IsTransacting)
	{
		FAIL_FAST_IF_WIN32_ERROR(DetourTransactionAbort());
	}
}
