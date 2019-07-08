#pragma once
#include <ShObjIdl.h>
#include <winrt/base.h>

class AppVisibilitySink : public winrt::implements<AppVisibilitySink, IAppVisibilityEvents> {

private:
	bool &m_startOpenedRef;

public:
	AppVisibilitySink(bool &startOpened);
	IFACEMETHODIMP LauncherVisibilityChange(BOOL currentVisibleState);
	IFACEMETHODIMP AppVisibilityOnMonitorChanged(HMONITOR, MONITOR_APP_VISIBILITY, MONITOR_APP_VISIBILITY);

};