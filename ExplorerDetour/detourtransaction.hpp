#pragma once
#include "arch.h"
#include <processthreadsapi.h>
#include <type_traits>
#include <windef.h>
#include <winerror.h>

#include "../detours/detours.h"
#include "detourexception.h"

class DetourTransaction {
private:
	bool m_IsTransacting;

public:
	inline DetourTransaction() : m_IsTransacting(false)
	{
		const LONG result = DetourTransactionBegin();
		if (result != NO_ERROR)
		{
			if (result == ERROR_INVALID_OPERATION)
			{
				throw DetourException(result, L"A pending transaction already exists.");
			}
			else
			{
				throw DetourException(result, L"Unknown error");
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
				throw DetourException(result, L"Not enough memory to record identity of thread.");
			}
			else
			{
				throw DetourException(result, L"Unknown error");
			}
		}
	}

	inline void update_current_thread()
	{
		update_thread(GetCurrentThread());
	}

	template<typename T>
	inline void attach(T& function, const T& detour)
	{
		static_assert(std::is_pointer_v<T> && std::is_function_v<std::remove_pointer_t<T>>, "T is not a function pointer");

		const LONG result = DetourAttach(reinterpret_cast<void **>(&function), reinterpret_cast<void *>(detour));
		if (result != NO_ERROR)
		{
			switch (result)
			{
			case ERROR_INVALID_BLOCK:
				throw DetourException(result, L"The function referenced is too small to be detoured.");

			case ERROR_INVALID_HANDLE:
				throw DetourException(result, L"The ppPointer parameter is null or points to a null pointer.");

			case ERROR_INVALID_OPERATION:
				throw DetourException(result, L"No pending transaction exists.");

			case ERROR_NOT_ENOUGH_MEMORY:
				throw DetourException(result, L"Not enough memory exists to complete the operation.");

			default:
				throw DetourException(result, L"Unknown error");
			}
		}
	}

	template<typename T>
	inline void detach(T &function, const T &detour)
	{
		static_assert(std::is_pointer_v<T> && std::is_function_v<std::remove_pointer_t<T>>, "T is not a function pointer");

		const LONG result = DetourDetach(reinterpret_cast<void **>(&function), reinterpret_cast<void *>(detour));
		if (result != NO_ERROR)
		{
			switch (result)
			{
			case ERROR_INVALID_BLOCK:
				throw DetourException(result, L"The function to be detached was too small to be detoured.");

			case ERROR_INVALID_HANDLE:
				throw DetourException(result, L"The ppPointer parameter is null or points to a null pointer.");

			case ERROR_INVALID_OPERATION:
				throw DetourException(result, L"No pending transaction exists.");

			case ERROR_NOT_ENOUGH_MEMORY:
				throw DetourException(result, L"Not enough memory exists to complete the operation.");

			default:
				throw DetourException(result, L"Unknown error");
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
				throw DetourException(result, L"Target function was changed by third party between steps of the transaction.");

			case ERROR_INVALID_OPERATION:
				throw DetourException(result, L"No pending transaction exists.");

			case ERROR_INVALID_BLOCK:
				throw DetourException(result, L"The function referenced is too small to be detoured.");

			case ERROR_INVALID_HANDLE:
				throw DetourException(result, L"The ppPointer parameter is null or points to a null pointer.");

			case ERROR_NOT_ENOUGH_MEMORY:
				throw DetourException(result, L"Not enough memory exists to complete the operation.");

			default:
				throw DetourException(result, L"Unknown error");
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
					throw DetourException(result, L"No pending transaction exists.");
				}
				else
				{
					throw DetourException(result, L"Unknown error");
				}
			}
		}
	}
};