#pragma once
#include "arch.h"
#include <ShObjIdl.h>
#include <type_traits>
#include "winrt.hpp"

class LauncherVisibilitySink : public winrt::implements<LauncherVisibilitySink, IAppVisibilityEvents> {
	using callback_t = std::add_pointer_t<void(void* , bool)>;

	callback_t m_Callback;
	void *m_Context;

	IFACEMETHODIMP LauncherVisibilityChange(BOOL currentVisibleState) noexcept override try
	{
		m_Callback(m_Context, currentVisibleState);
		return S_OK;
	}
	catch (...)
	{
		return winrt::to_hresult();
	}

	IFACEMETHODIMP AppVisibilityOnMonitorChanged(HMONITOR, MONITOR_APP_VISIBILITY, MONITOR_APP_VISIBILITY) noexcept override
	{
		return S_OK;
	}

public:
	inline LauncherVisibilitySink(callback_t callback, void *context) noexcept : m_Callback(callback), m_Context(context) { }
};
