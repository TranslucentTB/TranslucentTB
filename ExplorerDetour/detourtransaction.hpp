#pragma once
#include "arch.h"
#include <processthreadsapi.h>
#include <detours/detours.h>
#include <TlHelp32.h>
#include <type_traits>
#include <vector>
#include <wil/resource.h>
#include <windef.h>
#include <winerror.h>

#include "detourexception.hpp"
#include "util/concepts.hpp"

class DetourTransaction {
private:
	static constexpr Util::null_terminated_wstring_view PENDING_TRANSACTION_EXISTS = L"A pending transaction already exists.";
	static constexpr Util::null_terminated_wstring_view NO_PENDING_TRANSACTION = L"No pending transaction exists.";
	static constexpr Util::null_terminated_wstring_view FUNCTION_TOO_SMALL = L"The function referenced is too small to be detoured.";
	static constexpr Util::null_terminated_wstring_view FUNCTION_TOO_SMALL_DETACH = L"The function to be detached was too small to be detoured.";
	static constexpr Util::null_terminated_wstring_view OUT_OF_MEMORY = L"Not enough memory exists to complete the operation.";
	static constexpr Util::null_terminated_wstring_view OUT_OF_MEMORY_IDENTITY = L"Not enough memory to record identity of thread.";
	static constexpr Util::null_terminated_wstring_view NULL_POINTER = L"The ppPointer parameter is null or points to a null pointer.";
	static constexpr Util::null_terminated_wstring_view FUNCTION_TAMPERED = L"Target function was changed by third party between steps of the transaction.";
	static constexpr Util::null_terminated_wstring_view UNKNOWN_ERROR = L"Unknown error";
	static constexpr Util::null_terminated_wstring_view CREATE_SNAPSHOT_FAILED = L"CreateToolhelp32Snapshot failed.";
	static constexpr Util::null_terminated_wstring_view BEGIN_THREAD_ENUM_FAILED = L"Thread32First failed.";
	static constexpr Util::null_terminated_wstring_view OPEN_THREAD_FAILED = L"OpenThread failed.";

	bool m_IsTransacting;
	std::vector<wil::unique_handle> m_Threads;

public:
	inline DetourTransaction() : m_IsTransacting(false)
	{
		const LONG result = DetourTransactionBegin();
		if (result != NO_ERROR)
		{
			if (result == ERROR_INVALID_OPERATION)
			{
				throw DetourException(result, PENDING_TRANSACTION_EXISTS);
			}
			else
			{
				throw DetourException(result, UNKNOWN_ERROR);
			}
		}
		else
		{
			m_IsTransacting = true;
		}
	}

	inline DetourTransaction(const DetourTransaction &) = delete;
	inline DetourTransaction &operator =(const DetourTransaction &) = delete;

	inline void update_thread(wil::unique_handle hThread)
	{
		const LONG result = DetourUpdateThread(hThread.get());
		if (result != NO_ERROR)
		{
			hThread.reset();
			if (result == ERROR_NOT_ENOUGH_MEMORY)
			{
				throw DetourException(result, OUT_OF_MEMORY_IDENTITY);
			}
			else
			{
				throw DetourException(result, UNKNOWN_ERROR);
			}
		}
		else
		{
			m_Threads.push_back(std::move(hThread));
		}
	}

	inline void update_all_threads()
	{
		const DWORD pid = GetCurrentProcessId();
		wil::unique_tool_help_snapshot snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, pid));
		if (!snapshot)
		{
			throw DetourException(GetLastError(), CREATE_SNAPSHOT_FAILED);
		}

		THREADENTRY32 thread = { sizeof(thread) };
		if (!Thread32First(snapshot.get(), &thread))
		{
			throw DetourException(GetLastError(), BEGIN_THREAD_ENUM_FAILED);
		}

		const DWORD tid = GetCurrentThreadId();
		do
		{
			if (thread.th32OwnerProcessID == pid && thread.th32ThreadID != tid)
			{
				wil::unique_handle threadHandle(OpenThread(THREAD_QUERY_INFORMATION | THREAD_GET_CONTEXT | THREAD_SET_CONTEXT | THREAD_SUSPEND_RESUME, false, thread.th32ThreadID));
				if (!threadHandle)
				{
					throw DetourException(GetLastError(), OPEN_THREAD_FAILED);
				}
				update_thread(std::move(threadHandle));
			}
		}
		while (Thread32Next(snapshot.get(), &thread));
	}

	template<Util::function_pointer T>
	inline void attach(T &function, std::type_identity_t<T> detour)
	{
		const LONG result = DetourAttach(reinterpret_cast<void **>(&function), reinterpret_cast<void *>(detour));
		if (result != NO_ERROR)
		{
			switch (result)
			{
			case ERROR_INVALID_BLOCK:
				throw DetourException(result, FUNCTION_TOO_SMALL);

			case ERROR_INVALID_HANDLE:
				throw DetourException(result, NULL_POINTER);

			case ERROR_INVALID_OPERATION:
				throw DetourException(result, NO_PENDING_TRANSACTION);

			case ERROR_NOT_ENOUGH_MEMORY:
				throw DetourException(result, OUT_OF_MEMORY);

			default:
				throw DetourException(result, UNKNOWN_ERROR);
			}
		}
	}

	template<Util::function_pointer T>
	inline void detach(T &function, std::type_identity_t<T> detour)
	{
		const LONG result = DetourDetach(reinterpret_cast<void **>(&function), reinterpret_cast<void *>(detour));
		if (result != NO_ERROR)
		{
			switch (result)
			{
			case ERROR_INVALID_BLOCK:
				throw DetourException(result, FUNCTION_TOO_SMALL_DETACH);

			case ERROR_INVALID_HANDLE:
				throw DetourException(result, NULL_POINTER);

			case ERROR_INVALID_OPERATION:
				throw DetourException(result, NO_PENDING_TRANSACTION);

			case ERROR_NOT_ENOUGH_MEMORY:
				throw DetourException(result, OUT_OF_MEMORY);

			default:
				throw DetourException(result, UNKNOWN_ERROR);
			}
		}
	}

	inline void commit()
	{
		const LONG result = DetourTransactionCommit();
		if (result != NO_ERROR)
		{
			switch (result)
			{
			case ERROR_INVALID_DATA:
				throw DetourException(result, FUNCTION_TAMPERED);

			case ERROR_INVALID_OPERATION:
				throw DetourException(result, NO_PENDING_TRANSACTION);

			case ERROR_INVALID_BLOCK:
				throw DetourException(result, FUNCTION_TOO_SMALL);

			case ERROR_INVALID_HANDLE:
				throw DetourException(result, NULL_POINTER);

			case ERROR_NOT_ENOUGH_MEMORY:
				throw DetourException(result, OUT_OF_MEMORY);

			default:
				throw DetourException(result, UNKNOWN_ERROR);
			}
		}
		else
		{
			m_IsTransacting = false;
		}
	}

	inline ~DetourTransaction() noexcept(false)
	{
		if (m_IsTransacting)
		{
			const LONG result = DetourTransactionAbort();
			if (result != NO_ERROR)
			{
				if (result == ERROR_INVALID_OPERATION)
				{
					throw DetourException(result, NO_PENDING_TRANSACTION);
				}
				else
				{
					throw DetourException(result, UNKNOWN_ERROR);
				}
			}
		}
	}
};
