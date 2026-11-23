#include "visualtreewatcher.hpp"
#include <windows.ui.xaml.hosting.desktopwindowxamlsource.h>
#include "undefgetcurrenttime.h"
#include <winrt/Windows.UI.Xaml.Hosting.h>
#include "redefgetcurrenttime.h"

VisualTreeWatcher::VisualTreeWatcher(winrt::com_ptr<IUnknown> site, wil::unique_event_nothrow&& readyEvent) :
	m_XamlDiagnostics(site.as<IXamlDiagnostics>()),
	m_AppearanceService(winrt::make_self<TaskbarAppearanceService>()),
	m_ReadyEvent(std::move(readyEvent))
{

	// Calling AdviseVisualTreeChange from a separate thread solves some hangs.
	std::thread([self_strong = get_strong()]
	{
		// AdviseVisualTreeChange is special-cased to bring us to the UI thread for the callback.
		winrt::check_hresult(self_strong->m_XamlDiagnostics.as<IVisualTreeService3>()->AdviseVisualTreeChange(self_strong.get()));
		self_strong->m_ReadyEvent.SetEvent();
	}).detach();
}

HRESULT VisualTreeWatcher::OnVisualTreeChange(ParentChildRelation relation, VisualElement element, VisualMutationType mutationType) try
{
	// ownership of these strings is given to us lol
	wil::unique_bstr filename(element.SrcInfo.FileName);
	wil::unique_bstr hash(element.SrcInfo.Hash);
	wil::unique_bstr name(element.Name);
	wil::unique_bstr type(element.Type);

	switch (mutationType)
	{
	case Add:
	{
		const std::wstring_view type_view { type.get(), SysStringLen(type.get())};
		if (type_view == winrt::name_of<wuxh::DesktopWindowXamlSource>())
		{
			// we cannot check if the source contains a taskbar here,
			// because when a new taskbar gets added the source gets created
			// without containing anything initially. save the source to a set
			// of handles so we can later match it against the added TaskbarFrame.
			m_NonMatchingXamlSources.insert(element.Handle);
		}
		else if (type_view == L"Taskbar.TaskbarFrame")
		{
			HandleTaskbarFrame(relation, element);
		}
		else if (type_view == winrt::name_of<wux::Shapes::Rectangle>())
		{
			HandleBackgroundShape(relation, element, { name.get(), SysStringLen(name.get()) });
		}

		break;
	}

	case Remove: // only element.Handle is valid
		m_AppearanceService->UnregisterTaskbar(element.Handle);
		m_NonMatchingXamlSources.erase(element.Handle);
		break;
	}

	return S_OK;
}
catch (...)
{
	return winrt::to_hresult();
}

void VisualTreeWatcher::HandleTaskbarFrame(const ParentChildRelation& relation, const VisualElement& element)
{
	// we found a taskbar. match it up to the xaml source.
	wux::UIElement rootContent = nullptr;
	const auto frameElement = TryFromHandle<wux::UIElement>(element.Handle);
	if (frameElement)
	{
		try
		{
			if (const auto xamlRoot = frameElement.XamlRoot())
			{
				rootContent = xamlRoot.Content();
			}
		}
		catch (...)
		{
		}

		// Fallback 1: walk visual tree upwards if XamlRoot is not yet populated
		if (!rootContent)
		{
			wux::DependencyObject current = frameElement;
			wux::DependencyObject parent = nullptr;
			while ((parent = wux::Media::VisualTreeHelper::GetParent(current)) != nullptr)
			{
				current = parent;
			}
			rootContent = current.try_as<wux::UIElement>();
		}
	}

	// Fallback 2: direct relation.Parent (for older builds where TaskbarFrame is a direct child of RootGrid)
	const auto rootGrid = TryFromHandle<wux::UIElement>(relation.Parent);

	for (auto it = m_NonMatchingXamlSources.begin(); it != m_NonMatchingXamlSources.end(); ++it)
	{
		const auto xamlSource = TryFromHandle<wuxh::DesktopWindowXamlSource>(*it);
		if (!xamlSource)
		{
			continue;
		}

		wux::UIElement content = nullptr;
		try
		{
			content = xamlSource.Content();
		}
		catch (const winrt::hresult_wrong_thread&)
		{
			continue;
		}

		if ((rootContent && content == rootContent) || (rootGrid && content == rootGrid))
		{
			if (const auto nativeSource = xamlSource.try_as<IDesktopWindowXamlSourceNative>())
			{
				HWND hwnd = nullptr;
				if (SUCCEEDED(nativeSource->get_WindowHandle(&hwnd)) && hwnd)
				{
					m_AppearanceService->RegisterTaskbar(element.Handle, hwnd);
					m_NonMatchingXamlSources.erase(it);
					break;
				}
			}
		}
	}
}

void VisualTreeWatcher::HandleBackgroundShape(const ParentChildRelation& relation, const VisualElement& element, std::wstring_view name)
{
	const auto backgroundFill = name == L"BackgroundFill";
	const auto backgroundStroke = name == L"BackgroundStroke";
	if (!backgroundFill && !backgroundStroke)
	{
		return;
	}

	const auto frame = FindParent(L"TaskbarFrame", TryFromHandle<wux::FrameworkElement>(relation.Parent));
	if (!frame)
	{
		return;
	}

	InstanceHandle handle = 0;
	if (FAILED(m_XamlDiagnostics->GetHandleFromIInspectable(static_cast<::IInspectable*>(winrt::get_abi(frame)), &handle)) || handle == 0)
	{
		return;
	}

	const auto shape = TryFromHandle<wux::Shapes::Rectangle>(element.Handle);
	if (!shape)
	{
		return;
	}

	if (backgroundFill)
	{
		m_AppearanceService->RegisterTaskbarBackground(handle, shape);
	}
	else if (backgroundStroke)
	{
		m_AppearanceService->RegisterTaskbarBorder(handle, shape);
	}
}

HRESULT VisualTreeWatcher::OnElementStateChanged(InstanceHandle, VisualElementState, LPCWSTR) noexcept
{
	return S_OK;
}

wux::FrameworkElement VisualTreeWatcher::FindParent(std::wstring_view name, wux::FrameworkElement element)
{
	while (element)
	{
		if (element.Name() == name)
		{
			return element;
		}

		try
		{
			if (winrt::get_class_name(element) == L"Taskbar.TaskbarFrame")
			{
				return element;
			}
		}
		catch (...)
		{
		}

		element = wux::Media::VisualTreeHelper::GetParent(element).try_as<wux::FrameworkElement>();
	}

	return nullptr;
}
