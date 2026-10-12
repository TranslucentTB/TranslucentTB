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
			// Window 11 build 26340 collapsed the extra layer that used to sit between the
			// DesktopWindowXamlSource and the taskbar frame:
			//   older builds: DesktopWindowXamlSource -> RootGrid -> TaskbarFrame
			//   newer builds: DesktopWindowXamlSource -> TaskbarFrame
			// Matching only against the parent (the old RootGrid) therefore stopped working
			// and the taskbar was never registered, so no appearance was ever applied.
			// Accept either shape, and compare XAML diagnostics instance handles as well as
			// raw pointers: the diagnostics callback can hand back per-callback proxies whose
			// pointers do not compare equal for the same element.
			const auto frame = FromHandle<wux::FrameworkElement>(element.Handle);

			wux::UIElement parent = nullptr;
			if (frame)
			{
				parent = wux::Media::VisualTreeHelper::GetParent(frame).try_as<wux::UIElement>();
			}

			if (!parent)
			{
				parent = FromHandle<wux::UIElement>(relation.Parent);
			}

			const InstanceHandle parentHandle = HandleOf(parent);

			auto matched = m_NonMatchingXamlSources.end();
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

				const InstanceHandle contentHandle = HandleOf(content);
				const bool sameAsFrame = content == frame || (contentHandle != 0 && contentHandle == element.Handle);
				const bool sameAsParent = content == parent || (contentHandle != 0 && contentHandle == parentHandle);
				if (sameAsFrame || sameAsParent)
				{
					matched = it;
					break;
				}
			}

			if (matched != m_NonMatchingXamlSources.end())
			{
				const auto xamlSource = FromHandle<wuxh::DesktopWindowXamlSource>(*matched);
				const auto nativeSource = xamlSource.as<IDesktopWindowXamlSourceNative>();

				HWND hwnd = nullptr;
				winrt::check_hresult(nativeSource->get_WindowHandle(&hwnd));

				m_AppearanceService->RegisterTaskbar(element.Handle, hwnd);
				m_NonMatchingXamlSources.erase(matched);
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
