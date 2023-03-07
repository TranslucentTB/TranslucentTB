#include "visualtreewatcher.hpp"
#include "undefgetcurrenttime.h"
#include <winrt/Windows.UI.Xaml.Hosting.h>
#include "redefgetcurrenttime.h"
#include <windows.ui.xaml.hosting.desktopwindowxamlsource.h>

HRESULT VisualTreeWatcher::OnVisualTreeChange(ParentChildRelation, VisualElement element, VisualMutationType mutationType) try
{
	const std::wstring_view type { element.Type, SysStringLen(element.Type) };
	if (type == winrt::name_of<wuxh::DesktopWindowXamlSource>())
	{
		switch (mutationType)
		{
		case Add:
			if (!m_FoundSources.contains(element.Handle))
			{
				const auto desktopXamlSource = FromHandle<wuxh::DesktopWindowXamlSource>(element.Handle);

				// make sure we're on the taskbar control
				if (const auto taskbarFrame = FindControl(desktopXamlSource.Content().try_as<wux::FrameworkElement>(), L"TaskbarFrame"))
				{
					const auto backgroundControl = FindControl(taskbarFrame, L"BackgroundControl");
					const auto backgroundFill = FindControl(backgroundControl, L"BackgroundFill").as<wux::Shapes::Shape>();
					const auto backgroundStroke = FindControl(backgroundControl, L"BackgroundStroke").as<wux::Shapes::Shape>();
					if (backgroundFill && backgroundStroke)
					{
						const auto nativeSource = desktopXamlSource.as<IDesktopWindowXamlSourceNative>();

						HWND hwnd = nullptr;
						winrt::check_hresult(nativeSource->get_WindowHandle(&hwnd));

						if (const auto monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONULL))
						{
							m_FoundSources.insert_or_assign(element.Handle, TaskbarInfo {
								{
									backgroundFill,
									backgroundFill.Fill()
								},
								{
									backgroundStroke,
									backgroundStroke.Fill()
								},
								monitor
							});
						}
					}
				}
			}
			break;

		case Remove:
			if (auto it = m_FoundSources.find(element.Handle); it != m_FoundSources.end())
			{
				// TODO: would stuff still be alive here if we didn't keep a reference?
				RestoreElement(it->second.background);
				RestoreElement(it->second.border);

				m_FoundSources.erase(it);
			}
			break;
		}
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

void VisualTreeWatcher::SetXamlDiagnostics(winrt::com_ptr<IXamlDiagnostics> diagnostics)
{
	ClearSources();
	m_XamlDiagnostics = std::move(diagnostics);
}

VisualTreeWatcher::~VisualTreeWatcher()
{
	ClearSources();
}

void VisualTreeWatcher::ClearSources()
{
	for (const auto &[handle, info] : m_FoundSources)
	{
		RestoreElement(info.background);
		RestoreElement(info.border);
	}

	m_FoundSources.clear();
}

wux::FrameworkElement VisualTreeWatcher::FindControl(const wux::FrameworkElement &parent, std::wstring_view name)
{
	if (!parent)
	{
		return nullptr;
	}

	if (parent.Name() == name)
	{
		return parent;
	}

	const int32_t count = wux::Media::VisualTreeHelper::GetChildrenCount(parent);
	for (int32_t i = 0; i < count; i++)
	{
		if (const auto element = FindControl(wux::Media::VisualTreeHelper::GetChild(parent, i).try_as<wux::FrameworkElement>(), name))
		{
			return element;
		}
	}

	return nullptr;
}

void VisualTreeWatcher::RestoreElement(const ElementInfo<wux::Shapes::Shape> &element)
{
	element.element.Fill(element.originalFill);
}
