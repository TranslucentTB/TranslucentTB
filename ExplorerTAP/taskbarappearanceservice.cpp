#include "taskbarappearanceservice.hpp"
#include <RpcProxy.h>
#include <shellapi.h>
#include <wil/cppwinrt_helpers.h>

#include "constants.hpp"
#include "util/color.hpp"

extern "C"
{
	_Check_return_ HRESULT STDAPICALLTYPE DLLGETCLASSOBJECT_ENTRY(_In_ REFCLSID rclsid, _In_ REFIID riid, _Outptr_ void** ppv);
}

DWORD TaskbarAppearanceService::s_ProxyStubRegistrationCookie = 0;

TaskbarAppearanceService::TaskbarAppearanceService() :
	m_RegisterCookie(0),
	m_XamlThreadQueue(winrt::Windows::System::DispatcherQueue::GetForCurrentThread())
{
	InstallProxyStub();
	winrt::check_hresult(RegisterActiveObject(static_cast<ITaskbarAppearanceService*>(this), CLSID_TaskbarAppearanceService, ACTIVEOBJECT_STRONG, &m_RegisterCookie));
}

HRESULT TaskbarAppearanceService::GetVersion(DWORD* apiVersion) noexcept
{
	if (apiVersion)
	{
		*apiVersion = TAP_API_VERSION;
	}

	return S_OK;
}

HRESULT TaskbarAppearanceService::SetTaskbarAppearance(HWND taskbar, TaskbarBrush brush, UINT color) try
{
	for (const auto& [handle, info] : m_Taskbars)
	{
		if (GetAncestor(info.window, GA_PARENT) == taskbar)
		{
			if (info.background.control && info.background.originalFill)
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

				info.background.control.Fill(newBrush);
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

HRESULT TaskbarAppearanceService::ReturnTaskbarToDefaultAppearance(HWND taskbar) try
{
	for (const auto& [handle, info] : m_Taskbars)
	{
		if (GetAncestor(info.window, GA_PARENT) == taskbar)
		{
			RestoreDefaultControlFill(info.background);
			break;
		}
	}

	return S_OK;
}
catch (...)
{
	return winrt::to_hresult();
}

HRESULT TaskbarAppearanceService::SetTaskbarBorderVisibility(HWND taskbar, BOOL visible) try
{
	for (const auto& [handle, info] : m_Taskbars)
	{
		if (GetAncestor(info.window, GA_PARENT) == taskbar)
		{
			if (visible)
			{
				RestoreDefaultControlFill(info.border);
			}
			else
			{
				if (info.border.control && info.border.originalFill)
				{
					wux::Media::SolidColorBrush brush;
					brush.Opacity(0);
					info.border.control.Fill(brush);
				}
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

HRESULT TaskbarAppearanceService::RestoreAllTaskbarsToDefault() try
{
	for (const auto& [handle, info] : m_Taskbars)
	{
		RestoreDefaultControlFill(info.background);
		RestoreDefaultControlFill(info.border);
	}

	return S_OK;
}
catch (...)
{
	return winrt::to_hresult();
}

HRESULT TaskbarAppearanceService::RestoreAllTaskbarsToDefaultWhenProcessDies(DWORD pid)
{
	if (!m_Process)
	{
		m_Process.reset(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE, false, pid));
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

void TaskbarAppearanceService::RegisterTaskbar(InstanceHandle frameHandle, HWND window)
{
	m_Taskbars.insert_or_assign(frameHandle, TaskbarInfo { { }, { }, window });
}

void TaskbarAppearanceService::RegisterTaskbarBackground(InstanceHandle frameHandle, wux::Shapes::Shape element)
{
	if (const auto it = m_Taskbars.find(frameHandle); it != m_Taskbars.end())
	{
		it->second.background.control = element;
		it->second.background.originalFill = element.Fill();
	}
}

void TaskbarAppearanceService::RegisterTaskbarBorder(InstanceHandle frameHandle, wux::Shapes::Shape element)
{
	if (const auto it = m_Taskbars.find(frameHandle); it != m_Taskbars.end())
	{
		it->second.border.control = element;
		it->second.border.originalFill = element.Fill();
	}
}

void TaskbarAppearanceService::UnregisterTaskbar(InstanceHandle frameHandle)
{
	m_Taskbars.erase(frameHandle);
}

TaskbarAppearanceService::~TaskbarAppearanceService()
{
	// wait for all callbacks to be done, as they have a raw pointer to the instance.
	winrt::check_bool(UnregisterWaitEx(m_WaitHandle.get(), INVALID_HANDLE_VALUE));
	m_WaitHandle.release();

	m_Process.reset();

	winrt::check_hresult(RestoreAllTaskbarsToDefault());
	winrt::check_hresult(RevokeActiveObject(m_RegisterCookie, nullptr));
}

void TaskbarAppearanceService::InstallProxyStub()
{
	if (!s_ProxyStubRegistrationCookie)
	{
		winrt::com_ptr<IUnknown> proxyStub;
		winrt::check_hresult(DLLGETCLASSOBJECT_ENTRY(PROXY_CLSID_IS, winrt::guid_of<decltype(proxyStub)::type>(), proxyStub.put_void()));
		winrt::check_hresult(CoRegisterClassObject(PROXY_CLSID_IS, proxyStub.get(), CLSCTX_INPROC_SERVER, REGCLS_MULTIPLEUSE, &s_ProxyStubRegistrationCookie));

		winrt::check_hresult(CoRegisterPSClsid(IID_ITaskbarAppearanceService, PROXY_CLSID_IS));
		winrt::check_hresult(CoRegisterPSClsid(IID_IVersionedApi, PROXY_CLSID_IS));
	}
}

void TaskbarAppearanceService::UninstallProxyStub()
{
	if (s_ProxyStubRegistrationCookie)
	{
		winrt::check_hresult(CoRevokeClassObject(s_ProxyStubRegistrationCookie));
	}
}

winrt::fire_and_forget TaskbarAppearanceService::OnProcessDied()
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

void TaskbarAppearanceService::RestoreDefaultControlFill(const ControlInfo<wux::Shapes::Shape> &info)
{
	if (info.control && info.originalFill)
	{
		info.control.Fill(info.originalFill);
	}
}

void TaskbarAppearanceService::ProcessWaitCallback(void* parameter, BOOLEAN)
{
	reinterpret_cast<TaskbarAppearanceService *>(parameter)->OnProcessDied();
}
