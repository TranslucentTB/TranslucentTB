#pragma once
#include <string_view>
#include <unordered_set>
#include <xamlOM.h>
#include "winrt.hpp"
#include "undefgetcurrenttime.h"
#include <winrt/Windows.UI.Xaml.h>
#include "redefgetcurrenttime.h"

#include "taskbarappearanceservice.hpp"

class VisualTreeWatcher : public winrt::implements<VisualTreeWatcher, IVisualTreeServiceCallback2>
{
public:
	VisualTreeWatcher(winrt::com_ptr<IUnknown> site);

	VisualTreeWatcher(const VisualTreeWatcher&) = delete;
	VisualTreeWatcher& operator=(const VisualTreeWatcher&) = delete;

	VisualTreeWatcher(VisualTreeWatcher&&) = delete;
	VisualTreeWatcher& operator=(VisualTreeWatcher&&) = delete;

private:
	HRESULT STDMETHODCALLTYPE OnVisualTreeChange(ParentChildRelation relation, VisualElement element, VisualMutationType mutationType) override;
	HRESULT STDMETHODCALLTYPE OnElementStateChanged(InstanceHandle element, VisualElementState elementState, LPCWSTR context) noexcept override;

	wux::FrameworkElement FindParent(std::wstring_view name, wux::FrameworkElement element);

	template<typename T>
	T FromHandle(InstanceHandle handle)
	{
		wf::IInspectable obj;
		winrt::check_hresult(m_XamlDiagnostics->GetIInspectableFromHandle(handle, reinterpret_cast<::IInspectable**>(winrt::put_abi(obj))));

		return obj.as<T>();
	}

	winrt::com_ptr<IXamlDiagnostics> m_XamlDiagnostics;
	winrt::com_ptr<TaskbarAppearanceService> m_AppearanceService;
	std::unordered_set<InstanceHandle> m_NonMatchingXamlSources;
};
