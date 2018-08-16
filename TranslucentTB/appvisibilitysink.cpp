#include "appvisibilitysink.hpp"

AppVisibilitySink::AppVisibilitySink(bool &startOpened) : m_startOpenedRef(startOpened) { }

IFACEMETHODIMP AppVisibilitySink::LauncherVisibilityChange(BOOL currentVisibleState)
{
	m_startOpenedRef = currentVisibleState;
	return S_OK;
}

IFACEMETHODIMP AppVisibilitySink::AppVisibilityOnMonitorChanged(HMONITOR, MONITOR_APP_VISIBILITY, MONITOR_APP_VISIBILITY)
{
	return S_OK;
}