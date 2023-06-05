#include "visualtreewatcher.hpp"
#include "undefgetcurrenttime.h"
#include <winrt/Windows.System.h>
#include <winrt/Windows.UI.Xaml.Hosting.h>
#include "redefgetcurrenttime.h"
#include <windows.ui.xaml.hosting.desktopwindowxamlsource.h>
#include <wil/cppwinrt_helpers.h>

#include "constants.hpp"
#include "util/color.hpp"

DWORD VisualTreeWatcher::s_ProxyStubRegistrationCookie = 0;

VisualTreeWatcher::VisualTreeWatcher(winrt::com_ptr<IUnknown> site) :
	m_RegisterCookie(0),
	m_XamlDiagnostics(site.as<IXamlDiagnostics>()),
	m_XamlThreadQueue(winrt::Windows::System::DispatcherQueue::GetForCurrentThread())
{
}

void VisualTreeWatcher::InitializeComponent()
{
	const auto treeService = m_XamlDiagnostics.as<IVisualTreeService3>();

	winrt::check_hresult(treeService->AdviseVisualTreeChange(this));

	InstallProxyStub();
	const HRESULT hr = RegisterActiveObject(static_cast<ITaskbarAppearanceService*>(this), CLSID_VisualTreeWatcher, ACTIVEOBJECT_STRONG, &m_RegisterCookie);
	if (FAILED(hr)) [[unlikely]]
	{
		winrt::check_hresult(treeService->UnadviseVisualTreeChange(this));
		winrt::throw_hresult(hr);
	}
}

HRESULT VisualTreeWatcher::GetVersion(DWORD* apiVersion) noexcept
{
	if (apiVersion)
	{
		*apiVersion = TAP_API_VERSION;
	}

	return S_OK;
}

HRESULT VisualTreeWatcher::SetTaskbarAppearance(HMONITOR monitor, TaskbarBrush brush, UINT color) try
{
	for (const auto &[handle, info] : m_FoundSources)
	{
		if (info.monitor == monitor)
		{
			const winrt::Windows::UI::Color tint = Util::Color::FromABGR(color);
			wux::Media::Brush newBrush = nullptr;
			if (brush == Acrylic)
			{
				wux::Media::AcrylicBrush acrylicBrush;
				// on the taskbar, using Backdrop instead of HostBackdrop
				// makes the effect still show what's behind, but also not disable itself
				// when the window isn't active
				// this is because it sources what's behind the XAML, and the taskbar window
				// is transparent so what's behind is actually the content behind the window
				// (it doesn't need to poke a hole like HostBackdrop)
				acrylicBrush.BackgroundSource(wux::Media::AcrylicBackgroundSource::Backdrop);
				acrylicBrush.TintColor(tint);

				newBrush = std::move(acrylicBrush);
			}
			else if (brush == SolidColor)
			{
				wux::Media::SolidColorBrush solidBrush;
				solidBrush.Color(tint);

				newBrush = std::move(solidBrush);
			}

			info.background.element.Fill(newBrush);
			break;
		}
	}

	return S_OK;
}
catch (...)
{
	return winrt::to_hresult();
}

HRESULT VisualTreeWatcher::ReturnTaskbarToDefaultAppearance(HMONITOR monitor) try
{
	for (const auto &[handle, info] : m_FoundSources)
	{
		if (info.monitor == monitor)
		{
			RestoreElement(info.background);
			break;
		}
	}

	return S_OK;
}
catch (...)
{
	return winrt::to_hresult();
}

HRESULT VisualTreeWatcher::SetTaskbarBorderVisibility(HMONITOR monitor, BOOL visible) try
{
	for (const auto &[handle, info] : m_FoundSources)
	{
		if (info.monitor == monitor)
		{
			if (visible)
			{
				RestoreElement(info.border);
			}
			else
			{
				wux::Media::SolidColorBrush brush;
				brush.Opacity(0);
				info.border.element.Fill(brush);
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

HRESULT VisualTreeWatcher::RestoreAllTaskbarsToDefault() try
{
	for (const auto &[handle, info] : m_FoundSources)
	{
		RestoreElement(info.background);
		RestoreElement(info.border);
	}

	return S_OK;
}
catch (...)
{
	return winrt::to_hresult();
}

HRESULT VisualTreeWatcher::RestoreAllTaskbarsToDefaultWhenProcessDies(DWORD pid)
{
	if (!m_Process)
	{
		m_Process.reset(OpenProcess(SYNCHRONIZE, false, pid));
		if (!m_Process) [[unlikely]]
		{
			return HRESULT_FROM_WIN32(GetLastError());
		}

		if (!RegisterWaitForSingleObject(m_WaitHandle.put(), m_Process.get(), ProcessWaitCallback, this, INFINITE, WT_EXECUTEONLYONCE))
		{
			m_Process.reset();
			return HRESULT_FROM_WIN32(GetLastError());
		}

		return S_OK;
	}
	else
	{
		if (GetProcessId(m_Process.get()) == pid)
		{
			return S_OK; // already watching for this process to die.
		}
		else
		{
			return E_ILLEGAL_METHOD_CALL;
		}
	}
}

VisualTreeWatcher::~VisualTreeWatcher()
{
	// wait for all callbacks to be done, as they have a raw pointer to the instance.
	winrt::check_bool(UnregisterWaitEx(m_WaitHandle.get(), INVALID_HANDLE_VALUE));
	m_WaitHandle.release();

	m_Process.reset();

	winrt::check_hresult(RestoreAllTaskbarsToDefault());
	winrt::check_hresult(RevokeActiveObject(m_RegisterCookie, nullptr));
}

void VisualTreeWatcher::InstallProxyStub()
{
	if (!s_ProxyStubRegistrationCookie)
	{
		winrt::com_ptr<IUnknown> proxyStub;
		winrt::check_hresult(DllGetClassObject(PROXY_CLSID_IS, winrt::guid_of<decltype(proxyStub)::type>(), proxyStub.put_void()));
		winrt::check_hresult(CoRegisterClassObject(PROXY_CLSID_IS, proxyStub.get(), CLSCTX_INPROC_SERVER, REGCLS_MULTIPLEUSE, &s_ProxyStubRegistrationCookie));

		winrt::check_hresult(CoRegisterPSClsid(IID_ITaskbarAppearanceService, PROXY_CLSID_IS));
		winrt::check_hresult(CoRegisterPSClsid(IID_IVersionedApi, PROXY_CLSID_IS));
	}
}

void VisualTreeWatcher::UninstallProxyStub()
{
	if (s_ProxyStubRegistrationCookie)
	{
		winrt::check_hresult(CoRevokeClassObject(s_ProxyStubRegistrationCookie));
	}
}

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

winrt::fire_and_forget VisualTreeWatcher::OnProcessDied()
{
	m_WaitHandle.reset();
	m_Process.reset();

	const auto self_weak = get_weak();
	co_await wil::resume_foreground(m_XamlThreadQueue);

	if (const auto self = self_weak.get())
	{
		self->RestoreAllTaskbarsToDefault();
	}
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

void VisualTreeWatcher::ProcessWaitCallback(void* parameter, BOOLEAN)
{
	reinterpret_cast<VisualTreeWatcher*>(parameter)->OnProcessDied();
}
