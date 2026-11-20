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
			// assume it goes DesktopWindowXamlSource -> RootGrid -> TaskbarFrame.
			// we need RootGrid's pointer to find the right source based on its contents.
			if (!TryMatchFrame(element.Handle, relation.Parent))
			{
				// the source may not have its content set yet (or may not have been added yet),
				// which happens when a taskbar is created for an additional monitor. retry later
				// instead of dropping this taskbar forever.
				m_PendingFrames.insert_or_assign(element.Handle, relation.Parent);
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

		RetryPendingFrames();
		break;
	}

	case Remove: // only element.Handle is valid
		m_AppearanceService->UnregisterTaskbar(element.Handle);
		m_NonMatchingXamlSources.erase(element.Handle);
		m_PendingFrames.erase(element.Handle);
		break;
	}

	return S_OK;
}
catch (...)
{
	return winrt::to_hresult();
}

bool VisualTreeWatcher::TryMatchFrame(InstanceHandle frameHandle, InstanceHandle rootGridHandle)
{
	const auto rootGrid = FromHandle<wux::UIElement>(rootGridHandle);

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

		if (content && content == rootGrid)
		{
			const auto nativeSource = xamlSource.as<IDesktopWindowXamlSourceNative>();

			HWND hwnd = nullptr;
			winrt::check_hresult(nativeSource->get_WindowHandle(&hwnd));
			if (!hwnd)
			{
				// source isn't attached to its window yet, try again later.
				return false;
			}

			m_AppearanceService->RegisterTaskbar(frameHandle, hwnd);
			m_NonMatchingXamlSources.erase(it);

			return true;
		}
	}

	return false;
}

void VisualTreeWatcher::RetryPendingFrames()
{
	for (auto it = m_PendingFrames.begin(); it != m_PendingFrames.end();)
	{
		bool matched = false;
		try
		{
			matched = TryMatchFrame(it->first, it->second);
		}
		catch (...)
		{
			// the frame or its parent went away; forget about it.
			matched = true;
		}

		it = matched ? m_PendingFrames.erase(it) : std::next(it);
	}
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
