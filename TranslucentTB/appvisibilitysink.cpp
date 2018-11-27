#include "appvisibilitysink.hpp"

AppVisibilitySink::AppVisibilitySink(std::function<void(bool)> startOpenedCallback) : m_startOpenedCallback(std::move(startOpenedCallback)) { }

IFACEMETHODIMP AppVisibilitySink::LauncherVisibilityChange(BOOL currentVisibleState)
{
	m_startOpenedCallback(currentVisibleState);
	return S_OK;
}

IFACEMETHODIMP AppVisibilitySink::AppVisibilityOnMonitorChanged(HMONITOR, MONITOR_APP_VISIBILITY, MONITOR_APP_VISIBILITY)
{
	return S_OK;
}