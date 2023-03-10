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

struct VisualTreeWatcher : winrt::implements<VisualTreeWatcher, IVisualTreeServiceCallback2, IClassFactory, winrt::non_agile>
{
	VisualTreeWatcher() = default;
	void InitializeComponent();

	VisualTreeWatcher(const VisualTreeWatcher&) = delete;
	VisualTreeWatcher& operator=(const VisualTreeWatcher&) = delete;

	VisualTreeWatcher(VisualTreeWatcher&&) = delete;
	VisualTreeWatcher& operator=(VisualTreeWatcher&&) = delete;

	void SetXamlDiagnostics(winrt::com_ptr<IXamlDiagnostics> diagnostics);

	void SetTaskbarBrush(HMONITOR monitor, wux::Media::Brush fill);
	void RestoreOriginalTaskbarBrush(HMONITOR monitor);

	void ShowHideTaskbarBorder(HMONITOR monitor, bool visible);
	void RestoreAllTaskbars();

	~VisualTreeWatcher();

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
	HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppvObject) override;
	HRESULT STDMETHODCALLTYPE LockServer(BOOL) noexcept override;

	void ClearSources();

	template<typename T>
	T FromHandle(InstanceHandle handle)
	{
		wf::IInspectable obj;
		winrt::check_hresult(m_XamlDiagnostics->GetIInspectableFromHandle(handle, reinterpret_cast<::IInspectable**>(winrt::put_abi(obj))));

		return obj.as<T>();
	}

	static wux::FrameworkElement FindControl(const wux::FrameworkElement &parent, std::wstring_view name);
	static void RestoreElement(const ElementInfo<wux::Shapes::Shape> &element);

	winrt::com_ptr<IXamlDiagnostics> m_XamlDiagnostics = nullptr;
	std::unordered_map<InstanceHandle, TaskbarInfo> m_FoundSources;
};
