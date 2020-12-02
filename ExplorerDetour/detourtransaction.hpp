#pragma once
#include "arch.h"
#include <handleapi.h>
#include <memory>
#include <type_traits>
#include <wil/resource.h>

#include "detourresult.hpp"
#include "util/concepts.hpp"
#include "util/null_terminated_string_view.hpp"

class DetourTransaction {
private:
	static constexpr Util::null_terminated_wstring_view NO_PENDING_TRANSACTION = L"No pending transaction exists.";
	static constexpr Util::null_terminated_wstring_view FUNCTION_TOO_SMALL = L"The function referenced is too small to be detoured.";
	static constexpr Util::null_terminated_wstring_view OUT_OF_MEMORY = L"Not enough memory exists to complete the operation.";
	static constexpr Util::null_terminated_wstring_view NULL_POINTER = L"The ppPointer parameter is null or points to a null pointer.";
	static constexpr Util::null_terminated_wstring_view UNKNOWN_ERROR = L"Unknown error";

	struct node;
	struct node_deleter {
		void operator()(node *ptr) const noexcept;
	};
	using node_ptr = std::unique_ptr<node, node_deleter>;

	struct node {
		wil::unique_handle thread;
		node_ptr next;
	};

	node_ptr m_Head;
	bool m_IsTransacting = false;

	DetourResult attach_internal(void **function, void *detour) noexcept;
	DetourResult detach_internal(void **function, void *detour) noexcept;

public:
	constexpr DetourTransaction() noexcept = default;

	DetourTransaction(const DetourTransaction &) = delete;
	DetourTransaction &operator =(const DetourTransaction &) = delete;

	DetourResult begin() noexcept;
	DetourResult update_thread(wil::unique_handle hThread) noexcept;
	DetourResult update_all_threads() noexcept;
	DetourResult commit() noexcept;
	DetourResult abort() noexcept;

	~DetourTransaction() noexcept;

	template<Util::function_pointer T>
	DetourResult attach(T &function, std::type_identity_t<T> detour) noexcept
	{
		return attach_internal(reinterpret_cast<void **>(&function), reinterpret_cast<void *>(detour));
	}

	template<Util::function_pointer T>
	DetourResult detach(T &function, std::type_identity_t<T> detour) noexcept
	{
		return detach_internal(reinterpret_cast<void **>(&function), reinterpret_cast<void *>(detour));
	}
};
