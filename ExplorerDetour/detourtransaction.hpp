#pragma once
#include "arch.h"
#include <detours.h>
#include <processthreadsapi.h>
#include <type_traits>
#include <windef.h>
#include <winerror.h>

#include "detourexception.hpp"
#include "util/concepts.hpp"

class DetourTransaction {
private:
	static constexpr std::wstring_view PENDING_TRANSACTION_EXISTS = L"A pending transaction already exists.";
	static constexpr std::wstring_view NO_PENDING_TRANSACTION = L"No pending transaction exists.";
	static constexpr std::wstring_view FUNCTION_TOO_SMALL = L"The function referenced is too small to be detoured.";
	static constexpr std::wstring_view FUNCTION_TOO_SMALL_DETACH = L"The function to be detached was too small to be detoured.";
	static constexpr std::wstring_view OUT_OF_MEMORY = L"Not enough memory exists to complete the operation.";
	static constexpr std::wstring_view OUT_OF_MEMORY_IDENTITY = L"Not enough memory to record identity of thread.";
	static constexpr std::wstring_view NULL_POINTER = L"The ppPointer parameter is null or points to a null pointer.";
	static constexpr std::wstring_view FUNCTION_TAMPERED = L"Target function was changed by third party between steps of the transaction.";
	static constexpr std::wstring_view UNKNOWN_ERROR = L"Unknown error";

	bool m_IsTransacting;

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

	inline void update_thread(HANDLE hThread)
	{
		const LONG result = DetourUpdateThread(hThread);
		if (result != NO_ERROR)
		{
			if (result == ERROR_NOT_ENOUGH_MEMORY)
			{
				throw DetourException(result, OUT_OF_MEMORY_IDENTITY);
			}
			else
			{
				throw DetourException(result, UNKNOWN_ERROR);
			}
		}
	}

	inline void update_current_thread()
	{
		update_thread(GetCurrentThread());
	}

	template<Util::FunctionPointer T>
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

	template<Util::FunctionPointer T>
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
