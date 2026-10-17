#include "virtualdesktopmanager.hpp"
#include <utility>
#include <wil/resource.h>
#include <wrl/implements.h>
#include "win32.hpp"

namespace {
	class Sink final : public Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>, VirtualDesktopAbi::Notification> {
		HWND m_Window;
		UINT m_Message;
		HRESULT Changed() noexcept { PostMessageW(m_Window, m_Message, 0, 0); return S_OK; }
	public:
		Sink(HWND window, UINT message) noexcept : m_Window(window), m_Message(message) { }
		HRESULT STDMETHODCALLTYPE DesktopCreated(VirtualDesktopAbi::Desktop *) override { return Changed(); }
		HRESULT STDMETHODCALLTYPE DesktopDestroyBegin(VirtualDesktopAbi::Desktop *, VirtualDesktopAbi::Desktop *) override { return S_OK; }
		HRESULT STDMETHODCALLTYPE DesktopDestroyFailed(VirtualDesktopAbi::Desktop *, VirtualDesktopAbi::Desktop *) override { return Changed(); }
		HRESULT STDMETHODCALLTYPE DesktopDestroyed(VirtualDesktopAbi::Desktop *, VirtualDesktopAbi::Desktop *) override { return Changed(); }
		HRESULT STDMETHODCALLTYPE DesktopMoved(VirtualDesktopAbi::Desktop *, INT64, INT64) override { return Changed(); }
		HRESULT STDMETHODCALLTYPE DesktopNameChanged(VirtualDesktopAbi::Desktop *, HSTRING) override { return Changed(); }
		HRESULT STDMETHODCALLTYPE ViewDesktopChanged(IUnknown *) override { return S_OK; }
		HRESULT STDMETHODCALLTYPE CurrentDesktopChanged(VirtualDesktopAbi::Desktop *, VirtualDesktopAbi::Desktop *) override { return Changed(); }
		HRESULT STDMETHODCALLTYPE DesktopWallpaperChanged(VirtualDesktopAbi::Desktop *, HSTRING) override { return S_OK; }
		HRESULT STDMETHODCALLTYPE DesktopSwitched(VirtualDesktopAbi::Desktop *) override { return Changed(); }
		HRESULT STDMETHODCALLTYPE RemoteDesktopConnected(VirtualDesktopAbi::Desktop *) override { return Changed(); }
	};

	HRESULT ReadDesktopInfo(IObjectArray *desktops, UINT index, VirtualDesktopInfo &info)
	{
		Microsoft::WRL::ComPtr<VirtualDesktopAbi::Desktop> desktop;
		HRESULT hr = desktops->GetAt(index, IID_PPV_ARGS(&desktop));
		if (FAILED(hr)) return hr;
		if (!desktop) return E_UNEXPECTED;

		info.Position = index + 1;
		hr = desktop->GetId(&info.Id);
		if (FAILED(hr)) return hr;

		wil::unique_hstring name;
		if (SUCCEEDED(desktop->GetName(name.put())))
		{
			UINT length = 0;
			const auto text = WindowsGetStringRawBuffer(name.get(), &length);
			if (text)
			{
				info.Name.assign(text, length);
			}
		}
		if (info.Name.empty())
		{
			info.Name = L"Desktop " + std::to_wstring(info.Position);
		}
		return S_OK;
	}

	HRESULT GetCurrentDesktopId(VirtualDesktopAbi::Manager *manager, GUID &id)
	{
		Microsoft::WRL::ComPtr<VirtualDesktopAbi::Desktop> desktop;
		const HRESULT hr = manager->GetCurrentDesktop(&desktop);
		if (FAILED(hr)) return hr;
		if (!desktop) return E_UNEXPECTED;
		return desktop->GetId(&id);
	}

	HRESULT ReadDesktopSnapshot(VirtualDesktopAbi::Manager *manager,
		std::vector<VirtualDesktopInfo> &snapshot, std::optional<VirtualDesktopInfo> &active)
	{
		GUID currentId{};
		HRESULT hr = GetCurrentDesktopId(manager, currentId);
		if (FAILED(hr)) return hr;

		Microsoft::WRL::ComPtr<IObjectArray> desktops;
		hr = manager->GetDesktops(&desktops);
		if (FAILED(hr)) return hr;
		if (!desktops) return E_UNEXPECTED;

		UINT count = 0;
		hr = desktops->GetCount(&count);
		if (FAILED(hr)) return hr;
		for (UINT i = 0; i < count; ++i)
		{
			VirtualDesktopInfo info;
			hr = ReadDesktopInfo(desktops.Get(), i, info);
			if (FAILED(hr)) return hr;
			if (IsEqualGUID(info.Id, currentId))
			{
				active = info;
			}
			snapshot.push_back(std::move(info));
		}
		return active ? S_OK : HRESULT_FROM_WIN32(ERROR_RETRY);
	}
}

VirtualDesktopManager::~VirtualDesktopManager()
{
	Disconnect();
}

void VirtualDesktopManager::Disconnect() noexcept
{
	if (m_Service && m_Cookie)
	{
		m_Service->Unregister(*m_Cookie);
	}
	m_Cookie.reset();
	m_Sink.Reset();
	m_Service.Reset();
	m_Manager.Reset();
	m_Current.reset();
	m_Desktops.clear();
}

HRESULT VirtualDesktopManager::Connect(HWND window, UINT message)
{
	Disconnect();
	const auto [version, versionHr] = win32::GetWindowsBuild();
	if (FAILED(versionHr))
	{
		return m_LastError = versionHr;
	}
	if (!VirtualDesktopAbi::IsSupportedBuild(version.Build, version.Revision))
	{
		return m_LastError = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
	}
	Microsoft::WRL::ComPtr<IServiceProvider> shell;
	HRESULT hr = CoCreateInstance(VirtualDesktopAbi::ImmersiveShell, nullptr, CLSCTX_LOCAL_SERVER, IID_PPV_ARGS(&shell));
	if (SUCCEEDED(hr)) hr = shell->QueryService(VirtualDesktopAbi::ManagerService, IID_PPV_ARGS(&m_Manager));
	if (SUCCEEDED(hr)) hr = shell->QueryService(VirtualDesktopAbi::NotificationsService, IID_PPV_ARGS(&m_Service));
	if (SUCCEEDED(hr))
	{
		m_Sink = Microsoft::WRL::Make<Sink>(window, message);
		if (!m_Sink) hr = E_OUTOFMEMORY;
		else
		{
			DWORD cookie = 0;
			hr = m_Service->Register(m_Sink.Get(), &cookie);
			if (SUCCEEDED(hr)) m_Cookie = cookie;
		}
	}
	// Subscribe before querying so a switch during startup is not lost.
	if (SUCCEEDED(hr)) hr = Refresh();
	if (FAILED(hr))
	{
		Disconnect();
		m_LastError = hr;
	}
	return hr;
}

HRESULT VirtualDesktopManager::Refresh()
{
	if (!m_Manager)
	{
		return m_LastError = E_UNEXPECTED;
	}
	std::vector<VirtualDesktopInfo> snapshot;
	std::optional<VirtualDesktopInfo> active;
	const HRESULT hr = ReadDesktopSnapshot(m_Manager.Get(), snapshot, active);
	if (SUCCEEDED(hr))
	{
		m_Desktops = std::move(snapshot);
		m_Current = std::move(active);
	}
	else
	{
		m_Current.reset();
		m_Desktops.clear();
	}
	return m_LastError = hr;
}
