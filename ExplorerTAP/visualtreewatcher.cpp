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
			// On build 29695+ the taskbar nests the frame deeper:
			//   DesktopWindowXamlSource -> PopupRoot -> RootScrollViewer -> ... -> Border -> TaskbarFrame
			// Do not rely on "source content == frame's parent" anymore; match by XamlRoot instead.
			InstanceHandle matchedSource = FindSourceForFrame(FromHandle<wux::FrameworkElement>(element.Handle));

			// fallback: XamlRoot comparison inconclusive -> use the first pending source
			if (!matchedSource && !m_NonMatchingXamlSources.empty())
			{
				matchedSource = *m_NonMatchingXamlSources.begin();
			}

			if (matchedSource)
			{
				const auto nativeSource = FromHandle<wuxh::DesktopWindowXamlSource>(matchedSource).as<IDesktopWindowXamlSourceNative>();

				HWND hwnd = nullptr;
				winrt::check_hresult(nativeSource->get_WindowHandle(&hwnd));

				m_AppearanceService->RegisterTaskbar(element.Handle, hwnd);
				m_NonMatchingXamlSources.erase(matchedSource);
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

InstanceHandle VisualTreeWatcher::FindSourceForFrame(const wux::FrameworkElement& frame)
{
	wux::XamlRoot frameRoot = nullptr;
	try
	{
		frameRoot = frame.XamlRoot();
	}
	catch (...)
	{
		return 0;
	}

	if (!frameRoot)
	{
		return 0;
	}

	for (const auto handle : m_NonMatchingXamlSources)
	{
		try
		{
			const auto content = FromHandle<wuxh::DesktopWindowXamlSource>(handle).Content();
			const auto contentElement = content.try_as<wux::FrameworkElement>();
			if (contentElement && contentElement.XamlRoot() == frameRoot)
			{
				return handle;
			}
		}
		catch (...)
		{
			// the source may live on another thread; skip it
		}
	}

	return 0;
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