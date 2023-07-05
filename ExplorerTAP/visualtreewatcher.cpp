#include "visualtreewatcher.hpp"
#include <windows.ui.xaml.hosting.desktopwindowxamlsource.h>
#include "undefgetcurrenttime.h"
#include <winrt/Windows.UI.Xaml.Hosting.h>
#include "redefgetcurrenttime.h"

VisualTreeWatcher::VisualTreeWatcher(winrt::com_ptr<IUnknown> site) :
	m_XamlDiagnostics(site.as<IXamlDiagnostics>()),
	m_AppearanceService(winrt::make_self<TaskbarAppearanceService>())
{
	winrt::check_hresult(m_XamlDiagnostics.as<IVisualTreeService3>()->AdviseVisualTreeChange(this));
}

HRESULT VisualTreeWatcher::OnVisualTreeChange(ParentChildRelation relation, VisualElement element, VisualMutationType mutationType) try
{
	switch (mutationType)
	{
	case Add:
	{
		const std::wstring_view type { element.Type, SysStringLen(element.Type) };
		if (type == winrt::name_of<wuxh::DesktopWindowXamlSource>())
		{
			// we cannot check if the source contains a taskbar here,
			// because when a new taskbar gets added the source gets created
			// without containing anything initially. save the source to a set
			// of handles so we can later match it against the added TaskbarFrame.
			m_NonMatchingXamlSources.insert(element.Handle);
		}
		else if (type == L"Taskbar.TaskbarFrame")
		{
			// assume it goes DesktopWindowXamlSource -> RootGrid -> TaskbarFrame.
			// we need RootGrid's pointer to find the right source based on its contents.
			const auto rootGrid = FromHandle<wux::UIElement>(relation.Parent);

			for (auto it = m_NonMatchingXamlSources.begin(); it != m_NonMatchingXamlSources.end(); ++it)
			{
				const auto xamlSource = FromHandle<wuxh::DesktopWindowXamlSource>(*it);
				wux::UIElement content = nullptr;
				try
				{
					content = xamlSource.Content();
				}
				catch (const winrt::hresult_wrong_thread&)
				{
					continue;
				}

				if (content == rootGrid)
				{
					const auto nativeSource = xamlSource.as<IDesktopWindowXamlSourceNative>();

					HWND hwnd = nullptr;
					winrt::check_hresult(nativeSource->get_WindowHandle(&hwnd));

					m_AppearanceService->RegisterTaskbar(element.Handle, hwnd);
					m_NonMatchingXamlSources.erase(it);

					break;
				}
			}
		}
		else if (type == winrt::name_of<wux::Shapes::Rectangle>())
		{
			const std::wstring_view name { element.Name, SysStringLen(element.Name) };
			const auto backgroundFill = name == L"BackgroundFill";
			const auto backgroundStroke = name == L"BackgroundStroke";
			if (backgroundFill || backgroundStroke)
			{
				if (const auto frame = FindParent(L"TaskbarFrame", FromHandle<wux::FrameworkElement>(relation.Parent)))
				{
					InstanceHandle handle = 0;
					winrt::check_hresult(m_XamlDiagnostics->GetHandleFromIInspectable(reinterpret_cast<::IInspectable*>(winrt::get_abi(frame)), &handle));

					const auto shape = FromHandle<wux::Shapes::Rectangle>(element.Handle);
					if (backgroundFill)
					{
						m_AppearanceService->RegisterTaskbarBackground(handle, shape);
					}
					else if (backgroundStroke)
					{
						m_AppearanceService->RegisterTaskbarBorder(handle, shape);
					}
				}
			}
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

HRESULT VisualTreeWatcher::OnElementStateChanged(InstanceHandle, VisualElementState, LPCWSTR) noexcept
{
	return S_OK;
}

wux::FrameworkElement VisualTreeWatcher::FindParent(std::wstring_view name, wux::FrameworkElement element)
{
	const auto parent = wux::Media::VisualTreeHelper::GetParent(element).try_as<wux::FrameworkElement>();
	if (parent)
	{
		if (parent.Name() == name)
		{
			return parent;
		}
		else
		{
			return FindParent(name, parent);
		}
	}
	else
	{
		return nullptr;
	}
}
