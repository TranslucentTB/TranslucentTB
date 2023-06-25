#pragma once
#include <unordered_map>
#include <xamlOM.h>
#include "winrt.hpp"
#include "undefgetcurrenttime.h"
#include <winrt/Windows.System.h>
#include <winrt/Windows.UI.Xaml.Media.h>
#include <winrt/Windows.UI.Xaml.Shapes.h>
#include "redefgetcurrenttime.h"
#include <wil/resource.h>

#include "ExplorerTAP.h"
#include "wilx.hpp"

class TaskbarAppearanceService : public winrt::implements<TaskbarAppearanceService, ITaskbarAppearanceService, IVersionedApi, winrt::non_agile>
{
public:
	TaskbarAppearanceService();

	TaskbarAppearanceService(const TaskbarAppearanceService&) = delete;
	TaskbarAppearanceService& operator=(const TaskbarAppearanceService&) = delete;

	TaskbarAppearanceService(TaskbarAppearanceService&&) = delete;
	TaskbarAppearanceService& operator=(TaskbarAppearanceService&&) = delete;

	HRESULT STDMETHODCALLTYPE GetVersion(DWORD* apiVersion) noexcept override;

	HRESULT STDMETHODCALLTYPE SetTaskbarAppearance(HMONITOR monitor, TaskbarBrush brush, UINT color) override;
	HRESULT STDMETHODCALLTYPE ReturnTaskbarToDefaultAppearance(HMONITOR monitor) override;

	HRESULT STDMETHODCALLTYPE SetTaskbarBorderVisibility(HMONITOR monitor, BOOL visible) override;

	HRESULT STDMETHODCALLTYPE RestoreAllTaskbarsToDefault() override;
	HRESULT STDMETHODCALLTYPE RestoreAllTaskbarsToDefaultWhenProcessDies(DWORD pid) override;

	void RegisterTaskbar(InstanceHandle frameHandle, HWND window);
	void RegisterTaskbarBackground(InstanceHandle frameHandle, wux::Shapes::Shape element);
	void RegisterTaskbarBorder(InstanceHandle frameHandle, wux::Shapes::Shape element);
	void UnregisterTaskbar(InstanceHandle frameHandle);

	~TaskbarAppearanceService();

	static void InstallProxyStub();
	static void UninstallProxyStub();

private:
	template<typename T>
	struct ControlInfo
	{
		T control = nullptr;
		wux::Media::Brush originalFill = nullptr;
	};

	struct TaskbarInfo
	{
		ControlInfo<wux::Shapes::Shape> background, border;
		HWND window;
	};

	winrt::fire_and_forget OnProcessDied();

	static void RestoreDefaultControlFill(const ControlInfo<wux::Shapes::Shape> &info);
	static void NTAPI ProcessWaitCallback(void *parameter, BOOLEAN timedOut);

	DWORD m_RegisterCookie;
	std::unordered_map<InstanceHandle, TaskbarInfo> m_Taskbars;

	winrt::Windows::System::DispatcherQueue m_XamlThreadQueue;

	wil::unique_process_handle m_Process;
	wilx::unique_any<UnregisterWait> m_WaitHandle;

	static DWORD s_ProxyStubRegistrationCookie;
};
