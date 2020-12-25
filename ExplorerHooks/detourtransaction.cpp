#include "detourtransaction.hpp"
#include "arch.h"
#include <windef.h>
#include <winbase.h>
#include <handleapi.h>
#include <detours/detours.h>
#include <heapapi.h>
#include <new>
#include <processthreadsapi.h>
#include <TlHelp32.h>
#include <utility>

#include "swcadetour.hpp"
#include "util/abort.hpp"

void DetourTransaction::node_deleter::operator()(node *ptr) const noexcept
{
	ptr->~node();
	if (!HeapFree(SWCADetour::s_Heap, 0, ptr)) [[unlikely]]
	{
		Util::QuickAbort();
	}
}

DetourTransaction::node::~node() noexcept
{
	if (!CloseHandle(thread)) [[unlikely]]
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

DetourResult DetourTransaction::update_thread(HANDLE hThread) noexcept
{
	const LONG result = DetourUpdateThread(hThread);

	switch (result)
	{
	case NO_ERROR:
		if (const auto mem = HeapAlloc(SWCADetour::s_Heap, 0, sizeof(node)))
		{
			m_Head.reset(new (mem) node
			{
				.thread = hThread,
				.next = std::move(m_Head)
			});

			return { };
		}
		else
		{
			if (!CloseHandle(hThread)) [[unlikely]]
			{
				Util::QuickAbort();
			}

			return { L"HeapAlloc failed." };
		}

	case ERROR_NOT_ENOUGH_MEMORY:
		if (!CloseHandle(hThread)) [[unlikely]]
		{
			Util::QuickAbort();
		}

		return { result, L"Not enough memory to record identity of thread." };

	default:
		if (!CloseHandle(hThread)) [[unlikely]]
		{
			Util::QuickAbort();
		}

		return { result, UNKNOWN_ERROR };
	}
}

DetourResult DetourTransaction::update_all_threads() noexcept
{
	const DWORD pid = GetCurrentProcessId();
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, pid);
	if (!snapshot) [[unlikely]]
	{
		return { L"CreateToolhelp32Snapshot failed." };
	}

	THREADENTRY32 thread = { sizeof(thread) };
	if (!Thread32First(snapshot, &thread)) [[unlikely]]
	{
		if (!CloseHandle(snapshot)) [[unlikely]]
		{
			Util::QuickAbort();
		}

		return { L"Thread32First failed." };
	}

	const DWORD tid = GetCurrentThreadId();
	do
	{
		if (thread.th32OwnerProcessID == pid && thread.th32ThreadID != tid)
		{
			const HANDLE threadHandle = OpenThread(THREAD_QUERY_INFORMATION | THREAD_GET_CONTEXT | THREAD_SET_CONTEXT | THREAD_SUSPEND_RESUME, false, thread.th32ThreadID);
			if (!threadHandle) [[unlikely]]
			{
				if (!CloseHandle(snapshot)) [[unlikely]]
				{
					Util::QuickAbort();
				}

				return { L"OpenThread failed." };
			}

			const auto result = update_thread(threadHandle);
			if (!result) [[unlikely]]
			{
				if (!CloseHandle(snapshot)) [[unlikely]]
				{
					Util::QuickAbort();
				}

				return result;
			}
		}
	}
	while (Thread32Next(snapshot, &thread));

	if (!CloseHandle(snapshot)) [[unlikely]]
	{
		Util::QuickAbort();
	}

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
