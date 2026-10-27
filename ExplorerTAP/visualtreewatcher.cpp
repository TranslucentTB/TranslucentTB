#include "visualtreewatcher.hpp"
#include <windows.ui.xaml.hosting.desktopwindowxamlsource.h>
#include "undefgetcurrenttime.h"
#include <winrt/Windows.UI.Xaml.Hosting.h>
#include "redefgetcurrenttime.h"

namespace
{
	// Some taskbar builds put containers between the content of a
	// DesktopWindowXamlSource and the Taskbar.TaskbarFrame, so comparing the
	// frame's parent against the source content never matches. When that happens
	// the frame is never registered, and every appearance change becomes a no-op
	// that still returns S_OK.
	// The owning source can then be identified by its island window instead: the
	// taskbar's XAML island is parented to that monitor's Shell_TrayWnd.
	bool IsTaskbarIslandWindow(HWND window) noexcept
	{
		if (!window)
		{
			return false;
		}

		const HWND parent = GetAncestor(window, GA_PARENT);
		if (!parent)
		{
			return false;
		}

		wchar_t className[64];
		if (!GetClassNameW(parent, className, ARRAYSIZE(className)))
		{
			return false;
		}

		return lstrcmpW(className, L"Shell_TrayWnd") == 0;
	}
}

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
			// assume it goes DesktopWindowXamlSource -> RootGrid -> TaskbarFrame.
			// we need RootGrid's pointer to find the right source based on its contents.
			const auto rootGrid = FromHandle<wux::UIElement>(relation.Parent);

			InstanceHandle matchedHandle = 0;
			HWND matchedWindow = nullptr;
			bool matched = false;

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

					matchedHandle = *it;
					matchedWindow = hwnd;
					matched = true;
					break;
				}
			}

			// Fallback for taskbar builds where the content comparison above
			// never matches, which would otherwise leave the frame unregistered.
			if (!matched)
			{
				for (auto it = m_NonMatchingXamlSources.begin(); it != m_NonMatchingXamlSources.end(); ++it)
				{
					const auto xamlSource = FromHandle<wuxh::DesktopWindowXamlSource>(*it);
					const auto nativeSource = xamlSource.as<IDesktopWindowXamlSourceNative>();

					HWND hwnd = nullptr;
					if (FAILED(nativeSource->get_WindowHandle(&hwnd)))
					{
						continue;
					}

					if (IsTaskbarIslandWindow(hwnd))
					{
						matchedHandle = *it;
						matchedWindow = hwnd;
						matched = true;
						break;
					}
				}
			}

			if (matched)
			{
				m_AppearanceService->RegisterTaskbar(element.Handle, matchedWindow);
				m_NonMatchingXamlSources.erase(matchedHandle);
			}
		}
		else if (type_view == winrt::name_of<wux::Shapes::Rectangle>())
		{
			const std::wstring_view name_view { name.get(), SysStringLen(name.get())};
			const auto backgroundFill = name_view == L"BackgroundFill";
			const auto backgroundStroke = name_view == L"BackgroundStroke";
			if (backgroundFill || backgroundStroke)
			{
				if (const auto frame = FindParent(L"TaskbarFrame", FromHandle<wux::FrameworkElement>(relation.Parent)))
				{
					InstanceHandle handle = 0;
					winrt::check_hresult(m_XamlDiagnostics->GetHandleFromIInspectable(static_cast<::IInspectable*>(winrt::get_abi(frame)), &handle));

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
