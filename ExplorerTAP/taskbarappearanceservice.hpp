#pragma once
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <xamlOM.h>
#include "winrt.hpp"
#include "undefgetcurrenttime.h"
#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.UI.Xaml.Media.h>
#include <winrt/Windows.UI.Xaml.Shapes.h>
#include <winrt/Windows.UI.Xaml.Hosting.h>
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

	HRESULT STDMETHODCALLTYPE SetTaskbarAppearance(HWND taskbar, TaskbarBrush brush, UINT color) override;
	HRESULT STDMETHODCALLTYPE SetTaskbarBlur(HWND taskbar, UINT color, FLOAT blurAmount) override;
	HRESULT STDMETHODCALLTYPE ReturnTaskbarToDefaultAppearance(HWND taskbar) override;

	HRESULT STDMETHODCALLTYPE SetTaskbarBorderVisibility(HWND taskbar, BOOL visible) override;

	HRESULT STDMETHODCALLTYPE RestoreAllTaskbarsToDefault() override;
	HRESULT STDMETHODCALLTYPE RestoreAllTaskbarsToDefaultWhenProcessDies(DWORD pid) override;

	HRESULT STDMETHODCALLTYPE KillExplorerWhenPackageUninstalls(LPCWSTR packageFullName) override;

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
		int64_t fillChangedToken = 0;
	};

	struct TaskbarInfo
	{
		ControlInfo<wux::Shapes::Shape> background, border;
		HWND window;
	};

	winrt::fire_and_forget OnProcessDied();
	void OnPackageUninstalling(const wam::PackageCatalog &catalog, const wam::PackageUninstallingEventArgs &args);
	void OnPackageUpdating(const wam::PackageCatalog &catalog, const wam::PackageUpdatingEventArgs &args);
	std::optional<TaskbarInfo> GetTaskbarInfo(HWND taskbar) noexcept;

	void OnTaskbarBackgroundUpdated(const wux::DependencyObject &sender, const wux::DependencyProperty &dp);
	void OnTaskbarBorderUpdated(const wux::DependencyObject &sender, const wux::DependencyProperty &dp);

	static void RestoreDefaultControlFill(const ControlInfo<wux::Shapes::Shape> &info);
	static void NTAPI ProcessWaitCallback(void *parameter, BOOLEAN timedOut);

	void EnsureSubclass(HWND taskbar);
	static LRESULT CALLBACK TaskbarSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

	DWORD m_RegisterCookie;
	std::unordered_map<InstanceHandle, TaskbarInfo> m_Taskbars;
	std::unordered_set<HWND> m_SubclassedWindows;

	winrt::Windows::System::DispatcherQueue m_XamlThreadQueue;

	wil::unique_process_handle m_Process;
	wilx::unique_any<UnregisterWait> m_WaitHandle;

	wam::PackageCatalog m_PackageCatalog = nullptr;
	winrt::event_token m_PackageUninstallingToken;
	winrt::event_token m_PackageUpdatingToken;
	std::wstring m_PackageBeingWatched;

	static DWORD s_ProxyStubRegistrationCookie;
};
