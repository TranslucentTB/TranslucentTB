#include "detourtransaction.hpp"
#include "arch.h"
#include <windef.h>
#include <winbase.h>
#include <handleapi.h>
#include <detours/detours.h>
#include <new>
#include <processthreadsapi.h>
#include <utility>
#include <wil/result.h>

DetourTransaction::unique_hheap_failfast DetourTransaction::s_Heap;
const PSS_ALLOCATOR DetourTransaction::s_PssAllocator = {
	.AllocRoutine = PssAllocRoutineFailFast,
	.FreeRoutine = PssFreeRoutineFailFast
};

void DetourTransaction::HeapDestroyFailFast(HANDLE hHeap) noexcept
{
	FAIL_FAST_IF_WIN32_BOOL_FALSE(HeapDestroy(hHeap));
}

void DetourTransaction::PssFreeSnapshotFailFast(HPSS snapshot) noexcept
{
	FAIL_FAST_IF_WIN32_ERROR(PssFreeSnapshot(GetCurrentProcess(), snapshot));
}

void DetourTransaction::PssWalkMarkerFreeFailFast(HPSSWALK marker) noexcept
{
	FAIL_FAST_IF_WIN32_ERROR(PssWalkMarkerFree(marker));
}

void* DetourTransaction::PssAllocRoutineFailFast(void*, DWORD size) noexcept
{
	const auto mem = HeapAlloc(s_Heap.get(), 0, size);
	FAIL_FAST_LAST_ERROR_IF_NULL(mem);

	return mem;
}

void DetourTransaction::PssFreeRoutineFailFast(void*, void* address) noexcept
{
	FAIL_FAST_IF_WIN32_BOOL_FALSE(HeapFree(s_Heap.get(), 0, address));
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
	unique_hpss_failfast snapshot;
	FAIL_FAST_IF_WIN32_ERROR(PssCaptureSnapshot(GetCurrentProcess(), PSS_CAPTURE_THREADS, 0, snapshot.put()));

	unique_hpsswalk_failfast walker;
	FAIL_FAST_IF_WIN32_ERROR(PssWalkMarkerCreate(&s_PssAllocator, walker.put()));

	const auto pid = GetCurrentProcessId();
	const auto tid = GetCurrentThreadId();
	while (true)
	{
		PSS_THREAD_ENTRY thread;
		const DWORD error = PssWalkSnapshot(snapshot.get(), PSS_WALK_THREADS, walker.get(), &thread, sizeof(thread));
		if (error == ERROR_SUCCESS)
		{
			if (thread.ProcessId == pid && thread.ThreadId != tid && (thread.Flags & PSS_THREAD_FLAGS_TERMINATED) != PSS_THREAD_FLAGS_TERMINATED)
			{
				unique_handle_failfast threadHandle(OpenThread(THREAD_QUERY_INFORMATION | THREAD_GET_CONTEXT | THREAD_SET_CONTEXT | THREAD_SUSPEND_RESUME, false, thread.ThreadId));
				FAIL_FAST_LAST_ERROR_IF_NULL(threadHandle);

				update_thread(std::move(threadHandle));
			}
		}
		else if (error == ERROR_NO_MORE_ITEMS)
		{
			break;
		}
		else
		{
			FAIL_FAST_WIN32(error);
		}
	}
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
