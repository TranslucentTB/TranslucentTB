#pragma once
#include "arch.h"
#include <ShObjIdl.h>
#include "winrt.hpp"

#include "../windows/window.hpp"

class LauncherVisibilitySink : public winrt::implements<LauncherVisibilitySink, IAppVisibilityEvents> {
	Window m_Wnd;
	UINT m_Msg;

	IFACEMETHODIMP LauncherVisibilityChange(BOOL currentVisibleState) noexcept override
	{
		m_Wnd.send_message(m_Msg, currentVisibleState);
		return S_OK;
	}

	IFACEMETHODIMP AppVisibilityOnMonitorChanged(HMONITOR, MONITOR_APP_VISIBILITY, MONITOR_APP_VISIBILITY) noexcept override
	{
		return S_OK;
	}

public:
	LauncherVisibilitySink(Window wnd, UINT msg) noexcept : m_Wnd(wnd), m_Msg(msg) { }
};
