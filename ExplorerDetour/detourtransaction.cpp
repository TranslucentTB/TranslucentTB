#include "detourtransaction.hpp"
#include <detours/detours.h>
#include <heapapi.h>
#include <new>
#include <processthreadsapi.h>
#include <TlHelp32.h>
#include <utility>

#include "explorerdetour.hpp"
#include "util/abort.hpp"

void DetourTransaction::node_deleter::operator()(node *ptr) const noexcept
{
	ptr->~node();
	if (!HeapFree(ExplorerDetour::s_Heap.get(), 0, ptr))
	{
		Util::QuickAbort();
	}
}

DetourResult DetourTransaction::attach_internal(void** function, void* detour) noexcept
{
	const LONG result = DetourAttach(function, detour);

	switch (result)
	{
	case NO_ERROR:
		return { };

	case ERROR_INVALID_BLOCK:
		return { result, FUNCTION_TOO_SMALL };

	case ERROR_INVALID_HANDLE:
		return { result, NULL_POINTER };

	case ERROR_INVALID_OPERATION:
		return { result, NO_PENDING_TRANSACTION };

	case ERROR_NOT_ENOUGH_MEMORY:
		return { result, OUT_OF_MEMORY };

	default:
		return { result, UNKNOWN_ERROR };
	}
}

DetourResult DetourTransaction::detach_internal(void **function, void *detour) noexcept
{
	const LONG result = DetourDetach(function, detour);

	switch (result)
	{
	case NO_ERROR:
		return { };

	case ERROR_INVALID_BLOCK:
		return { result, L"The function to be detached was too small to be detoured." };

	case ERROR_INVALID_HANDLE:
		return { result, NULL_POINTER };

	case ERROR_INVALID_OPERATION:
		return { result, NO_PENDING_TRANSACTION };

	case ERROR_NOT_ENOUGH_MEMORY:
		return { result, OUT_OF_MEMORY };

	default:
		return { result, UNKNOWN_ERROR };
	}
}

DetourResult DetourTransaction::begin() noexcept
{
	const LONG result = DetourTransactionBegin();

	switch (result)
	{
	case NO_ERROR:
		m_IsTransacting = true;
		return { };

	case ERROR_INVALID_OPERATION:
		return { result, L"A pending transaction already exists." };

	default:
		return { result, UNKNOWN_ERROR };
	}
}

DetourResult DetourTransaction::update_thread(wil::unique_handle hThread) noexcept
{
	const LONG result = DetourUpdateThread(hThread.get());

	switch (result)
	{
	case NO_ERROR:
		if (const auto mem = HeapAlloc(ExplorerDetour::s_Heap.get(), 0, sizeof(node)))
		{
			m_Head.reset(new (mem) node
			{
				.thread = std::move(hThread),
				.next = std::move(m_Head)
			});

			return { };
		}
		else
		{
			return { L"HeapAlloc failed." };
		}

	case ERROR_NOT_ENOUGH_MEMORY:
		return { result, L"Not enough memory to record identity of thread." };

	default:
		return { result, UNKNOWN_ERROR };
	}
}

DetourResult DetourTransaction::update_all_threads() noexcept
{
	const DWORD pid = GetCurrentProcessId();
	wil::unique_tool_help_snapshot snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, pid));
	if (!snapshot)
	{
		return { L"CreateToolhelp32Snapshot failed." };
	}

	THREADENTRY32 thread = { sizeof(thread) };
	if (!Thread32First(snapshot.get(), &thread))
	{
		return { L"Thread32First failed." };
	}

	const DWORD tid = GetCurrentThreadId();
	do
	{
		if (thread.th32OwnerProcessID == pid && thread.th32ThreadID != tid)
		{
			wil::unique_handle threadHandle(OpenThread(THREAD_QUERY_INFORMATION | THREAD_GET_CONTEXT | THREAD_SET_CONTEXT | THREAD_SUSPEND_RESUME, false, thread.th32ThreadID));
			if (!threadHandle)
			{
				return { L"OpenThread failed." };
			}

			const auto result = update_thread(std::move(threadHandle));
			if (!result)
			{
				return result;
			}
		}
	}
	while (Thread32Next(snapshot.get(), &thread));

	return { };
}

DetourResult DetourTransaction::commit() noexcept
{
	const LONG result = DetourTransactionCommit();

	switch (result)
	{
	case NO_ERROR:
		m_IsTransacting = false;
		return { };

	case ERROR_INVALID_DATA:
		return { result, L"Target function was changed by third party between steps of the transaction." };

	case ERROR_INVALID_OPERATION:
		return { result, NO_PENDING_TRANSACTION };

	case ERROR_INVALID_BLOCK:
		return { result, FUNCTION_TOO_SMALL };

	case ERROR_INVALID_HANDLE:
		return { result, NULL_POINTER };

	case ERROR_NOT_ENOUGH_MEMORY:
		return { result, OUT_OF_MEMORY };

	default:
		return { result, UNKNOWN_ERROR };
	}
}

DetourResult DetourTransaction::abort() noexcept
{
	const LONG result = DetourTransactionAbort();

	switch (result)
	{
	case NO_ERROR:
		m_IsTransacting = false;
		return { };

	case ERROR_INVALID_OPERATION:
		return { result, NO_PENDING_TRANSACTION };

	default:
		return { result, UNKNOWN_ERROR };
	}
}

DetourTransaction::~DetourTransaction() noexcept
{
	if (m_IsTransacting && !abort())
	{
		Util::QuickAbort();
	}
}
