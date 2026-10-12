#pragma once
#include <string_view>
#include <unordered_set>
#include <xamlOM.h>
#include "winrt.hpp"
#include "undefgetcurrenttime.h"
#include <winrt/Windows.UI.Xaml.h>
#include "redefgetcurrenttime.h"

#include "taskbarappearanceservice.hpp"

struct VisualTreeWatcher : winrt::implements<VisualTreeWatcher, IVisualTreeServiceCallback2, winrt::non_agile>
{
public:
	VisualTreeWatcher(winrt::com_ptr<IUnknown> site, wil::unique_event_nothrow&& readyEvent);

	VisualTreeWatcher(const VisualTreeWatcher&) = delete;
	VisualTreeWatcher& operator=(const VisualTreeWatcher&) = delete;

	VisualTreeWatcher(VisualTreeWatcher&&) = delete;
	VisualTreeWatcher& operator=(VisualTreeWatcher&&) = delete;

private:
	HRESULT STDMETHODCALLTYPE OnVisualTreeChange(ParentChildRelation relation, VisualElement element, VisualMutationType mutationType) override;
	HRESULT STDMETHODCALLTYPE OnElementStateChanged(InstanceHandle element, VisualElementState elementState, LPCWSTR context) noexcept override;

	wux::FrameworkElement FindParent(std::wstring_view name, wux::FrameworkElement element);

	// Resolves an element to its XAML diagnostics instance handle. Handles are stable
	// identifiers, unlike raw interface pointers which may refer to per-callback proxies.
	InstanceHandle HandleOf(const wux::UIElement& element) noexcept
	{
		if (!element)
		{
			return 0;
		}

		InstanceHandle handle = 0;
		if (FAILED(m_XamlDiagnostics->GetHandleFromIInspectable(static_cast<::IInspectable*>(winrt::get_abi(element)), &handle)))
		{
			return 0;
		}

		return handle;
	}

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
	wil::unique_event_nothrow m_ReadyEvent; // keep a hold of the event so it stays set
};
