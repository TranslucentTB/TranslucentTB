#pragma once
#include "arch.h"
#include <ShObjIdl.h>
#include <type_traits>
#include "winrt.hpp"

template<auto Callback, typename T>
class LauncherVisibilitySink : public winrt::implements<LauncherVisibilitySink<Callback, T>, IAppVisibilityEvents> {
	T* context;

	IFACEMETHODIMP LauncherVisibilityChange(BOOL currentVisibleState) noexcept override try
	{
		std::invoke(Callback, context, currentVisibleState);
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
	constexpr LauncherVisibilitySink(T* context) noexcept : context(context) { }
};
