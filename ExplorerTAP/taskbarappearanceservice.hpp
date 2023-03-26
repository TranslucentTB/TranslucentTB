#pragma once
#include "arch.h"
#include <windef.h>
#include "winrt.hpp"

#include "ExplorerTAP.h"
#include "visualtreewatcher.hpp"

class TaskbarAppearanceService :
	public winrt::implements<TaskbarAppearanceService, ITaskbarAppearanceService, winrt::non_agile> {
public:
	TaskbarAppearanceService(winrt::com_ptr<VisualTreeWatcher> watcher);

	TaskbarAppearanceService(const TaskbarAppearanceService&) = delete;
	TaskbarAppearanceService& operator=(const TaskbarAppearanceService&) = delete;

	TaskbarAppearanceService(TaskbarAppearanceService&&) = delete;
	TaskbarAppearanceService& operator=(TaskbarAppearanceService&&) = delete;

	static void Install(IClassFactory* classFactory);
	static void InstallProxyStub();
	static void Uninstall();
	static void UninstallProxyStub();

private:
	static DWORD s_RegistrationCookie;
	static DWORD s_ProxyStubRegistrationCookie;

	HRESULT STDMETHODCALLTYPE SetTaskbarAppearance(HMONITOR monitor, TaskbarBrush brush, UINT color) override;
	HRESULT STDMETHODCALLTYPE ReturnTaskbarToDefaultAppearance(HMONITOR monitor) override;

	HRESULT STDMETHODCALLTYPE SetTaskbarBorderVisibility(HMONITOR monitor, BOOL visible) override;

	HRESULT STDMETHODCALLTYPE RestoreAllTaskbarsToDefault() override;

	winrt::com_ptr<VisualTreeWatcher> m_Watcher;
};
