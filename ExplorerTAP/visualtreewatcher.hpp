#pragma once
#include <string_view>
#include <unordered_set>
#include <xamlOM.h>
#include "winrt.hpp"
#include "undefgetcurrenttime.h"
#include <winrt/Windows.UI.Xaml.h>
#include "redefgetcurrenttime.h"

struct VisualTreeWatcher : winrt::implements<VisualTreeWatcher, IVisualTreeServiceCallback2>
{
	HRESULT STDMETHODCALLTYPE OnVisualTreeChange(ParentChildRelation relation, VisualElement element, VisualMutationType mutationType) override;
	HRESULT STDMETHODCALLTYPE OnElementStateChanged(InstanceHandle element, VisualElementState elementState, LPCWSTR context) noexcept override;

	void SetXamlDiagnostics(winrt::com_ptr<IXamlDiagnostics> diagnostics) noexcept;

private:
	template<typename T>
	T FromHandle(InstanceHandle handle)
	{
		wf::IInspectable obj;
		winrt::check_hresult(m_XamlDiagnostics->GetIInspectableFromHandle(handle, reinterpret_cast<::IInspectable**>(winrt::put_abi(obj))));

		return obj.as<T>();
	}

	static wux::FrameworkElement FindControl(const wux::FrameworkElement &parent, std::wstring_view name);

	winrt::com_ptr<IXamlDiagnostics> m_XamlDiagnostics;
	std::unordered_set<InstanceHandle> m_FoundSources;
};
