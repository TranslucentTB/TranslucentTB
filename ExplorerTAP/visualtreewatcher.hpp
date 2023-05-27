#pragma once
#include <string_view>
#include <unordered_set>
#include <xamlOM.h>
#include "winrt.hpp"
#include "undefgetcurrenttime.h"
#include <winrt/Windows.UI.Xaml.h>
#include <winrt/Windows.UI.Xaml.Media.h>
#include <winrt/Windows.UI.Xaml.Shapes.h>
#include "redefgetcurrenttime.h"

#include "ExplorerTAP.h"

class VisualTreeWatcher : public winrt::implements<VisualTreeWatcher, ITaskbarAppearanceService, IVisualTreeServiceCallback2, winrt::non_agile>
{
public:
	VisualTreeWatcher(winrt::com_ptr<IUnknown> site);
	void InitializeComponent();

	VisualTreeWatcher(const VisualTreeWatcher&) = delete;
	VisualTreeWatcher& operator=(const VisualTreeWatcher&) = delete;

	VisualTreeWatcher(VisualTreeWatcher&&) = delete;
	VisualTreeWatcher& operator=(VisualTreeWatcher&&) = delete;

	HRESULT STDMETHODCALLTYPE SetTaskbarAppearance(HMONITOR monitor, TaskbarBrush brush, UINT color) override;
	HRESULT STDMETHODCALLTYPE ReturnTaskbarToDefaultAppearance(HMONITOR monitor) override;

	HRESULT STDMETHODCALLTYPE SetTaskbarBorderVisibility(HMONITOR monitor, BOOL visible) override;

	HRESULT STDMETHODCALLTYPE RestoreAllTaskbarsToDefault() override;

	~VisualTreeWatcher();

	static void InstallProxyStub();
	static void UninstallProxyStub();

private:
	template<typename T>
	struct ElementInfo
	{
		T element;
		wux::Media::Brush originalFill;
	};

	struct TaskbarInfo
	{
		ElementInfo<wux::Shapes::Shape> background, border;
		HMONITOR monitor;
	};

	HRESULT STDMETHODCALLTYPE OnVisualTreeChange(ParentChildRelation relation, VisualElement element, VisualMutationType mutationType) override;
	HRESULT STDMETHODCALLTYPE OnElementStateChanged(InstanceHandle element, VisualElementState elementState, LPCWSTR context) noexcept override;

	template<typename T>
	T FromHandle(InstanceHandle handle)
	{
		wf::IInspectable obj;
		winrt::check_hresult(m_XamlDiagnostics->GetIInspectableFromHandle(handle, reinterpret_cast<::IInspectable**>(winrt::put_abi(obj))));

		return obj.as<T>();
	}

	static wux::FrameworkElement FindControl(const wux::FrameworkElement &parent, std::wstring_view name);
	static void RestoreElement(const ElementInfo<wux::Shapes::Shape> &element);

	DWORD m_RegisterCookie;
	winrt::com_ptr<IXamlDiagnostics> m_XamlDiagnostics;
	std::unordered_map<InstanceHandle, TaskbarInfo> m_FoundSources;

	static DWORD s_ProxyStubRegistrationCookie;
};
