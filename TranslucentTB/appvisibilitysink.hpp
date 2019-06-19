#pragma once
#include <functional>
#include <ShObjIdl.h>
#include <winrt/base.h>

class AppVisibilitySink : public winrt::implements<AppVisibilitySink, IAppVisibilityEvents> {
private:
	std::function<void(bool)> m_startOpenedCallback;

public:
	inline AppVisibilitySink(std::function<void(bool)> startOpenedCallback) :
		m_startOpenedCallback(std::move(startOpenedCallback))
	{ }

	IFACEMETHODIMP LauncherVisibilityChange(BOOL currentVisibleState) override
	{
		try
		{
			m_startOpenedCallback(currentVisibleState);
			return S_OK;
		}
		catch (...)
		{
			return winrt::to_hresult();
		}
	}

	IFACEMETHODIMP AppVisibilityOnMonitorChanged(HMONITOR, MONITOR_APP_VISIBILITY, MONITOR_APP_VISIBILITY) override
	{
		return S_OK;
	}
};