#pragma once
#include "arch.h"
#include <memory>
#include <type_traits>
#include <windef.h>
#include <processsnapshot.h>
#include <wil/resource.h>

#include "common.hpp"
#include "util/concepts.hpp"
#include "util/null_terminated_string_view.hpp"
#include "wilx.hpp"

class DetourTransaction {
private:
	static void HeapDestroyFailFast(HANDLE hHeap) noexcept;
	using unique_hheap_failfast = wilx::unique_any<HeapDestroyFailFast>;

	static void PssFreeSnapshotFailFast(HPSS snapshot) noexcept;
	using unique_hpss_failfast = wilx::unique_any<PssFreeSnapshotFailFast>;

	static void PssWalkMarkerFreeFailFast(HPSSWALK marker) noexcept;
	using unique_hpsswalk_failfast = wilx::unique_any<PssWalkMarkerFreeFailFast>;

	static void* WINAPI PssAllocRoutineFailFast(void* context, DWORD size) noexcept;
	static void WINAPI PssFreeRoutineFailFast(void* context, void* address) noexcept;

	struct node;
	struct node_deleter {
		void operator()(node *ptr) const noexcept;
	};
	using node_ptr = std::unique_ptr<node, node_deleter>;

	struct node {
		unique_handle_failfast thread;
		node_ptr next;
	};

	static unique_hheap_failfast s_Heap;
	static const PSS_ALLOCATOR s_PssAllocator;

	node_ptr m_Head;
	bool m_IsTransacting = false;

	void attach_internal(void **function, void *detour) noexcept;
	void detach_internal(void **function, void *detour) noexcept;

	void update_thread(unique_handle_failfast hThread) noexcept;

public:
	DetourTransaction() noexcept;

	DetourTransaction(const DetourTransaction &) = delete;
	DetourTransaction &operator =(const DetourTransaction &) = delete;

	void update_all_threads() noexcept;
	void commit() noexcept;

	~DetourTransaction() noexcept;

	template<Util::function_pointer T>
	void attach(T &function, std::type_identity_t<T> detour) noexcept
	{
		attach_internal(reinterpret_cast<void **>(&function), reinterpret_cast<void *>(detour));
	}

	template<Util::function_pointer T>
	void detach(T &function, std::type_identity_t<T> detour) noexcept
	{
		detach_internal(reinterpret_cast<void **>(&function), reinterpret_cast<void *>(detour));
	}
};
