#include "taskbarappearanceservice.hpp"
#include "util/color.hpp"

DWORD TaskbarAppearanceService::s_RegistrationCookie = 0;
DWORD TaskbarAppearanceService::s_ProxyStubRegistrationCookie = 0;

TaskbarAppearanceService::TaskbarAppearanceService(winrt::com_ptr<VisualTreeWatcher> watcher) :
	m_Watcher(std::move(watcher))
{
}

void TaskbarAppearanceService::Install(IClassFactory* classFactory)
{
	winrt::check_hresult(CoRegisterClassObject(
		CLSID_TaskbarAppearanceService,
		classFactory,
		CLSCTX_LOCAL_SERVER,
		REGCLS_MULTIPLEUSE,
		&s_RegistrationCookie
	));

	InstallProxyStub();
}

void TaskbarAppearanceService::InstallProxyStub()
{
	if (!s_ProxyStubRegistrationCookie)
	{
		winrt::com_ptr<IUnknown> proxyStub;
		winrt::check_hresult(DllGetClassObject(PROXY_CLSID_IS, winrt::guid_of<decltype(proxyStub)::type>(), proxyStub.put_void()));
		winrt::check_hresult(CoRegisterClassObject(PROXY_CLSID_IS, proxyStub.get(), CLSCTX_INPROC_SERVER, REGCLS_MULTIPLEUSE, &s_ProxyStubRegistrationCookie));

		winrt::check_hresult(CoRegisterPSClsid(IID_ITaskbarAppearanceService, PROXY_CLSID_IS));
	}
}

void TaskbarAppearanceService::Uninstall()
{
	winrt::check_hresult(CoRevokeClassObject(s_RegistrationCookie));
}

void TaskbarAppearanceService::UninstallProxyStub()
{
	if (s_ProxyStubRegistrationCookie)
	{
		winrt::check_hresult(CoRevokeClassObject(s_ProxyStubRegistrationCookie));
	}
}

HRESULT TaskbarAppearanceService::SetTaskbarAppearance(HMONITOR monitor, TaskbarBrush brush, UINT color) try
{
	wux::Media::Brush fill = nullptr;
	const winrt::Windows::UI::Color tint = Util::Color::FromABGR(color);
	if (brush == Acrylic)
	{
		wux::Media::AcrylicBrush acrylicBrush;
		acrylicBrush.BackgroundSource(wux::Media::AcrylicBackgroundSource::HostBackdrop);
		acrylicBrush.TintColor(tint);

		fill = std::move(acrylicBrush);
	}
	else if (brush == SolidColor)
	{
		wux::Media::SolidColorBrush solidBrush;
		solidBrush.Color(tint);

		fill = std::move(solidBrush);
	}

	m_Watcher->SetTaskbarBrush(monitor, fill);

	return S_OK;
}
catch (...)
{
	return winrt::to_hresult();
}

HRESULT TaskbarAppearanceService::ReturnTaskbarToDefaultAppearance(HMONITOR monitor) try
{
	m_Watcher->RestoreOriginalTaskbarBrush(monitor);
	return S_OK;
}
catch (...)
{
	return winrt::to_hresult();
}

HRESULT TaskbarAppearanceService::SetTaskbarBorderVisibility(HMONITOR monitor, BOOL visible) try
{
	m_Watcher->ShowHideTaskbarBorder(monitor, visible);
	return S_OK;
}
catch (...)
{
	return winrt::to_hresult();
}

HRESULT TaskbarAppearanceService::RestoreAllTaskbarsToDefault() try
{
	m_Watcher->RestoreAllTaskbars();
	return S_OK;
}
catch (...)
{
	return winrt::to_hresult();
}
