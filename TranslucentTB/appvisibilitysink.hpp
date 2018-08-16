#pragma once
#include <ShObjIdl.h>
#include <wrl/implements.h>

// Cannot use winrt::implements here because of https://bugs.llvm.org/show_bug.cgi?id=38490
class AppVisibilitySink : public Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>, IAppVisibilityEvents> {

private:
	bool &m_startOpenedRef;

public:
	AppVisibilitySink(bool &startOpened);
	IFACEMETHODIMP LauncherVisibilityChange(BOOL currentVisibleState);
	IFACEMETHODIMP AppVisibilityOnMonitorChanged(HMONITOR, MONITOR_APP_VISIBILITY, MONITOR_APP_VISIBILITY);

};