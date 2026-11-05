#include "taskbarappearanceservice.hpp"
#include <CommCtrl.h>
#include <algorithm>
#include <RpcProxy.h>
#include <shellapi.h>
#include <windows.ui.xaml.hosting.desktopwindowxamlsource.h>
#include <wil/cppwinrt_helpers.h>
#include <winrt/Windows.Management.Deployment.h>

#include "constants.hpp"
#include "util/color.hpp"

#include "XamlBlurBrush.h"

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
	if (const auto info = GetTaskbarInfo(taskbar))
	{
		if (info->background.control && info->background.originalFill)
		{
			EnsureSubclass(taskbar);

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

			info->background.control.Fill(newBrush);
		}
	}

	return S_OK;
}
catch (...)
{
	return winrt::to_hresult();
}

HRESULT TaskbarAppearanceService::SetTaskbarBlur(HWND taskbar, UINT color, FLOAT blurAmount) try
{
	if (const auto info = GetTaskbarInfo(taskbar))
	{
		if (info->background.control && info->background.originalFill)
		{
			EnsureSubclass(taskbar);

			const winrt::Windows::UI::Color tint = Util::Color::FromABGR(color);
			const wfn::float4 tintHdr = {
				tint.R / 255.0f,
				tint.G / 255.0f,
				tint.B / 255.0f,
				tint.A / 255.0f
			};

			auto compositor = wuxh::ElementCompositionPreview::GetElementVisual(info->background.control).Compositor();
			info->background.control.Fill(winrt::make<XamlBlurBrush>(std::move(compositor), blurAmount, tintHdr));
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
	if (const auto info = GetTaskbarInfo(taskbar))
	{
		RestoreDefaultControlFill(info->background);
	}

	return S_OK;
}
catch (...)
{
	return winrt::to_hresult();
}

HRESULT TaskbarAppearanceService::SetTaskbarBorderVisibility(HWND taskbar, BOOL visible) try
{
	if (const auto info = GetTaskbarInfo(taskbar))
	{
		if (visible)
		{
			RestoreDefaultControlFill(info->border);
		}
		else if (info->border.control && info->border.originalFill)
		{
			wux::Media::SolidColorBrush brush;
			brush.Opacity(0);
			info->border.control.Fill(brush);
		}
	}

	return S_OK;
}
catch (...)
{
	return winrt::to_hresult();
}

HRESULT TaskbarAppearanceService::SetTaskbarTextColor(HWND taskbar, UINT color) try
{
	ApplyTrayFlyoutTheme();

	for (auto& [handle, info] : m_Taskbars)
	{
		if (GetAncestor(info.window, GA_PARENT) != taskbar)
		{
			continue;
		}

		const auto textColor = Util::Color::FromABGR(color);
		if (textColor.A == 0)
		{
			RestoreDefaultTextColor(info);
			break;
		}

		if (!info.root || info.textColor == color)
		{
			break;
		}

		if (!info.originalTheme)
		{
			info.originalTheme = info.root.RequestedTheme();
		}

		// pick the theme the text color fits best in, so that what we don't recolor
		// (system tray glyphs, hover backgrounds, etc.) matches the text.
		const double luminance = 0.2126 * textColor.R + 0.7152 * textColor.G + 0.0722 * textColor.B;
		info.theme = luminance > 127.5 ? wux::ElementTheme::Dark : wux::ElementTheme::Light;

		// explorer sets the theme again when the system tells it the theme changed (ImmersiveColorSet).
		if (!info.themeChangedToken)
		{
			info.themeChangedToken = info.root.RegisterPropertyChangedCallback(wux::FrameworkElement::RequestedThemeProperty(), [weak = get_weak(), frameHandle = handle](const wux::DependencyObject &, const wux::DependencyProperty &)
			{
				if (const auto self = weak.get())
				{
					self->OnRootThemeUpdated(frameHandle);
				}
			});
		}

		info.root.RequestedTheme(*info.theme);

		// pure white and pure black are what the dark and light themes already use, so for these
		// only fix up what explorer colors by itself based on the system theme.
		RestoreDefaultTextForeground(info);
		info.overrideAllText = !(textColor == Util::Color { 0xFF, 0xFF, 0xFF } || textColor == Util::Color { 0x00, 0x00, 0x00 });
		info.textBrush = wux::Media::SolidColorBrush(textColor);
		ApplyTextColor(handle, info, info.root);

		info.textColor = color;
		break;
	}

	return S_OK;
}
catch (...)
{
	return winrt::to_hresult();
}

HRESULT TaskbarAppearanceService::SetTrayIconColor(UINT color) try
{
	m_TrayIconColor = (color >> 24) != 0 ? color : 0;
	ApplyTrayFlyoutTheme();

	return m_TrayIconRecolor.SetColor(color, m_Process ? GetProcessId(m_Process.get()) : 0) ? S_OK : HRESULT_FROM_WIN32(ERROR_INVALID_THREAD_ID);
}
catch (...)
{
	return winrt::to_hresult();
}

HRESULT TaskbarAppearanceService::RestoreAllTaskbarsToDefault() try
{
	for (auto& [handle, info] : m_Taskbars)
	{
		RestoreDefaultControlFill(info.background);
		RestoreDefaultControlFill(info.border);
		RestoreDefaultTextColor(info);
	}

	m_TrayIconColor = 0;
	ApplyTrayFlyoutTheme();
	m_TrayIconRecolor.SetColor(0, m_Process ? GetProcessId(m_Process.get()) : 0);

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

HRESULT TaskbarAppearanceService::KillExplorerWhenPackageUninstalls(LPCWSTR packageFullName) try
{
	const std::wstring_view packageFullNameSv { packageFullName };

	if (!m_PackageCatalog)
	{
		winrt::Windows::Management::Deployment::PackageManager pm;
		auto catalog = wam::PackageCatalog::OpenForPackage(pm.FindPackageForUser(L"", packageFullNameSv));

		m_PackageUninstallingToken = catalog.PackageUninstalling({ get_weak(), &TaskbarAppearanceService::OnPackageUninstalling });
		m_PackageUpdatingToken = catalog.PackageUpdating({ get_weak(), &TaskbarAppearanceService::OnPackageUpdating });
		m_PackageBeingWatched = packageFullNameSv;
		m_PackageCatalog = std::move(catalog);

		return S_OK;
	}
	else
	{
		if (m_PackageBeingWatched == packageFullNameSv)
		{
			return S_OK; // already watching for this package to get uninstalled.
		}
		else
		{
			return E_ILLEGAL_METHOD_CALL;
		}
	}
}
catch (...)
{
	return winrt::to_hresult();
}

void TaskbarAppearanceService::RegisterTaskbar(InstanceHandle frameHandle, HWND window, wux::FrameworkElement root)
{
	TaskbarInfo info;
	info.window = window;
	info.root = std::move(root);

	m_Taskbars.insert_or_assign(frameHandle, std::move(info));
}

void TaskbarAppearanceService::RegisterTaskbarBackground(InstanceHandle frameHandle, wux::Shapes::Shape element)
{
	if (const auto it = m_Taskbars.find(frameHandle); it != m_Taskbars.end())
	{
		it->second.background.control = element;

		// sometimes we may see objects come with their fill set to null, wait until the system initialized it before
		// we start messing with it
		// in the future if the taskbar doesn't change the original brush's color, but tries to swap the entire brush
		// this could be useful to suppress it and update the saved brush
		element.RegisterPropertyChangedCallback(wux::Shapes::Shape::FillProperty(), { get_weak(), &TaskbarAppearanceService::OnTaskbarBackgroundUpdated });
		it->second.background.originalFill = element.Fill();
	}
}

void TaskbarAppearanceService::RegisterTaskbarBorder(InstanceHandle frameHandle, wux::Shapes::Shape element)
{
	if (const auto it = m_Taskbars.find(frameHandle); it != m_Taskbars.end())
	{
		it->second.border.control = element;

		element.RegisterPropertyChangedCallback(wux::Shapes::Shape::FillProperty(), { get_weak(), &TaskbarAppearanceService::OnTaskbarBorderUpdated });
		it->second.border.originalFill = element.Fill();
	}
}

void TaskbarAppearanceService::RegisterTextBlock(wuxc::TextBlock element)
{
	// fast path: nothing to do unless a custom text color is active.
	if (std::ranges::none_of(m_Taskbars, [](const auto &pair) { return pair.second.textBrush != nullptr; }))
	{
		return;
	}

	wux::DependencyObject current = element;
	while (current)
	{
		for (auto& [handle, info] : m_Taskbars)
		{
			if (info.textBrush && info.root == current)
			{
				ApplyTextColor(handle, info, element);
				return;
			}
		}

		current = wux::Media::VisualTreeHelper::GetParent(current);
	}
}

void TaskbarAppearanceService::RegisterXamlSource(const wuxh::DesktopWindowXamlSource &source)
{
	std::erase_if(m_XamlSources, [](const XamlSourceInfo &info) { return info.source.get() == nullptr; });
	m_XamlSources.push_back({ winrt::make_weak(source) });
}

void TaskbarAppearanceService::ApplyTrayFlyoutTheme()
{
	// the hidden icons flyout is its own XAML island with the system theme, recolored icons
	// would be invisible on it, so give it the theme matching the icon color.
	const int r = m_TrayIconColor & 0xFF, g = (m_TrayIconColor >> 8) & 0xFF, b = (m_TrayIconColor >> 16) & 0xFF;
	const auto theme = (0.2126 * r + 0.7152 * g + 0.0722 * b) > 127.5 ? wux::ElementTheme::Dark : wux::ElementTheme::Light;

	for (auto &info : m_XamlSources)
	{
		try
		{
			const auto source = info.source.get();
			if (!source)
			{
				continue;
			}

			HWND hwnd = nullptr;
			winrt::check_hresult(source.as<IDesktopWindowXamlSourceNative>()->get_WindowHandle(&hwnd));

			wchar_t className[64] {};
			if (!hwnd || !GetClassNameW(GetAncestor(hwnd, GA_ROOT), className, static_cast<int>(std::size(className))) ||
				std::wstring_view(className) != L"TopLevelWindowForOverflowXamlIsland")
			{
				continue;
			}

			const auto root = source.Content().try_as<wux::FrameworkElement>();
			if (root != info.root)
			{
				info.root = root;
				info.originalTheme.reset();
			}

			if (!root)
			{
				continue;
			}

			if (m_TrayIconColor != 0)
			{
				if (!info.originalTheme)
				{
					info.originalTheme = root.RequestedTheme();
				}

				root.RequestedTheme(theme);
			}
			else if (info.originalTheme)
			{
				root.RequestedTheme(*info.originalTheme);
				info.originalTheme.reset();
			}
		}
		catch (...)
		{
			// source on another thread or being torn down
		}
	}
}

void TaskbarAppearanceService::UnregisterTaskbar(InstanceHandle frameHandle)
{
	if (auto it = m_Taskbars.find(frameHandle); it != m_Taskbars.end())
	{
		try
		{
			RestoreDefaultTextColor(it->second);
		}
		catch (...)
		{
			// the tree is being torn down, nothing to restore.
		}


		const auto fillProperty = wux::Shapes::Shape::FillProperty();
		if (it->second.background.fillChangedToken != 0)
		{
			it->second.background.control.UnregisterPropertyChangedCallback(fillProperty, it->second.background.fillChangedToken);
		}

		if (it->second.border.fillChangedToken != 0)
		{
			it->second.border.control.UnregisterPropertyChangedCallback(fillProperty, it->second.border.fillChangedToken);
		}

		m_Taskbars.erase(it);
	}
}

TaskbarAppearanceService::~TaskbarAppearanceService()
{
	if (m_PackageCatalog)
	{
		m_PackageCatalog.PackageUpdating(m_PackageUpdatingToken);
		m_PackageCatalog.PackageUninstalling(m_PackageUninstallingToken);

		m_PackageBeingWatched.clear();
		m_PackageUpdatingToken = { };
		m_PackageUninstallingToken = { };
		m_PackageCatalog = nullptr;
	}

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

void TaskbarAppearanceService::OnPackageUninstalling(const wam::PackageCatalog&, const wam::PackageUninstallingEventArgs &args)
{
	if (args.Package().Id().FullName() == m_PackageBeingWatched)
	{
		// kill explorer to force the system to restart it, thereby unloading our DLL.
		TerminateProcess(GetCurrentProcess(), 0);
	}
}

void TaskbarAppearanceService::OnPackageUpdating(const wam::PackageCatalog&, const wam::PackageUpdatingEventArgs &args)
{
	if (args.SourcePackage().Id().FullName() == m_PackageBeingWatched)
	{
		// kill explorer to force the system to restart it, thereby unloading our DLL.
		TerminateProcess(GetCurrentProcess(), 0);
	}
}

std::optional<TaskbarAppearanceService::TaskbarInfo> TaskbarAppearanceService::GetTaskbarInfo(HWND taskbar) noexcept
{
	for (const auto& [handle, info] : m_Taskbars)
	{
		if (GetAncestor(info.window, GA_PARENT) == taskbar)
		{
			return info;
		}
	}

	return std::nullopt;
}

void TaskbarAppearanceService::OnTaskbarBackgroundUpdated(const wux::DependencyObject &sender, const wux::DependencyProperty&)
{
	for (auto& [handle, info] : m_Taskbars)
	{
		if (sender == info.background.control && !info.background.originalFill)
		{
			info.background.originalFill = info.background.control.Fill();
		}
	}
}

void TaskbarAppearanceService::OnTaskbarBorderUpdated(const wux::DependencyObject &sender, const wux::DependencyProperty&)
{
	for (auto& [handle, info] : m_Taskbars)
	{
		if (sender == info.border.control && !info.border.originalFill)
		{
			info.border.originalFill = info.border.control.Fill();
		}
	}
}

void TaskbarAppearanceService::OnTextForegroundUpdated(InstanceHandle frameHandle, const wux::DependencyObject &sender)
{
	if (const auto it = m_Taskbars.find(frameHandle); it != m_Taskbars.end() && it->second.textBrush)
	{
		auto &info = it->second;
		const auto textBlock = sender.as<wuxc::TextBlock>();
		if (textBlock.Foreground() != info.textBrush)
		{
			// explorer changed the color on its own, remember it as the new original and override it again if needed.
			for (auto &text : info.texts)
			{
				if (text.control.get() == textBlock)
				{
					text.originalForeground = textBlock.ReadLocalValue(wuxc::TextBlock::ForegroundProperty());
					break;
				}
			}

			if (ShouldOverrideText(info, textBlock))
			{
				textBlock.Foreground(info.textBrush);
			}
		}
	}
}

void TaskbarAppearanceService::OnRootThemeUpdated(InstanceHandle frameHandle)
{
	if (const auto it = m_Taskbars.find(frameHandle); it != m_Taskbars.end())
	{
		auto &info = it->second;
		if (info.root && info.theme)
		{
			if (const auto current = info.root.RequestedTheme(); current != *info.theme)
			{
				// remember what explorer wants for when we stop overriding it.
				info.originalTheme = current;
				info.root.RequestedTheme(*info.theme);
			}
		}
	}
}

void TaskbarAppearanceService::ApplyTextColor(InstanceHandle frameHandle, TaskbarInfo &info, const wux::DependencyObject &element)
{
	if (const auto textBlock = element.try_as<wuxc::TextBlock>())
	{
		ApplyTextColor(frameHandle, info, textBlock);
	}

	const int32_t count = wux::Media::VisualTreeHelper::GetChildrenCount(element);
	for (int32_t i = 0; i < count; ++i)
	{
		ApplyTextColor(frameHandle, info, wux::Media::VisualTreeHelper::GetChild(element, i));
	}
}

void TaskbarAppearanceService::ApplyTextColor(InstanceHandle frameHandle, TaskbarInfo &info, const wuxc::TextBlock &textBlock)
{
	if (textBlock.Foreground() == info.textBrush ||
		std::ranges::any_of(info.texts, [&textBlock](const TextInfo &existing) { return existing.control.get() == textBlock; }))
	{
		return; // already handled (elements can get re-added to the tree)
	}

	const auto foregroundProperty = wuxc::TextBlock::ForegroundProperty();

	TextInfo text;
	text.control = winrt::make_weak(textBlock);
	text.originalForeground = textBlock.ReadLocalValue(foregroundProperty);
	if (ShouldOverrideText(info, textBlock))
	{
		textBlock.Foreground(info.textBrush);
	}

	// keep watching even text blocks we left alone, explorer may change their color later.
	text.foregroundChangedToken = textBlock.RegisterPropertyChangedCallback(foregroundProperty, [weak = get_weak(), frameHandle](const wux::DependencyObject &sender, const wux::DependencyProperty &)
	{
		if (const auto self = weak.get())
		{
			self->OnTextForegroundUpdated(frameHandle, sender);
		}
	});

	// drop text blocks that were destroyed since.
	std::erase_if(info.texts, [](const TextInfo &existing) { return existing.control.get() == nullptr; });
	info.texts.push_back(std::move(text));
}

void TaskbarAppearanceService::RestoreDefaultControlFill(const ControlInfo<wux::Shapes::Shape> &info)
{
	if (info.control && info.originalFill)
	{
		info.control.Fill(info.originalFill);
	}
}

bool TaskbarAppearanceService::ShouldOverrideText(const TaskbarInfo &info, const wuxc::TextBlock &textBlock)
{
	const auto local = textBlock.ReadLocalValue(wuxc::TextBlock::ForegroundProperty()).try_as<wux::Media::SolidColorBrush>();
	if (!local)
	{
		// colored by the theme, which we already switched.
		return info.overrideAllText;
	}

	const auto color = local.Color();
	const int r = color.R, g = color.G, b = color.B;
	if ((std::max)({ r, g, b }) - (std::min)({ r, g, b }) > 48)
	{
		return false; // a meaningful color (charging battery, accent...), leave it alone.
	}

	if (info.overrideAllText)
	{
		return true;
	}

	// explorer picked black or white from the system theme by itself, flip it if it's the wrong one.
	const auto target = info.textBrush.Color();
	const bool lightTarget = 0.2126 * target.R + 0.7152 * target.G + 0.0722 * target.B > 127.5;
	const bool lightText = 0.2126 * r + 0.7152 * g + 0.0722 * b > 127.5;
	return lightTarget != lightText;
}

void TaskbarAppearanceService::RestoreDefaultTextColor(TaskbarInfo &info)
{
	RestoreDefaultTextForeground(info);

	info.theme.reset();
	if (info.root && info.themeChangedToken)
	{
		info.root.UnregisterPropertyChangedCallback(wux::FrameworkElement::RequestedThemeProperty(), info.themeChangedToken);
	}
	info.themeChangedToken = 0;

	if (info.root && info.originalTheme)
	{
		info.root.RequestedTheme(*info.originalTheme);
	}

	info.originalTheme.reset();
	info.textColor = 0;
}

void TaskbarAppearanceService::RestoreDefaultTextForeground(TaskbarInfo &info)
{
	const auto foregroundProperty = wuxc::TextBlock::ForegroundProperty();
	for (const auto &text : info.texts)
	{
		if (const auto textBlock = text.control.get())
		{
			textBlock.UnregisterPropertyChangedCallback(foregroundProperty, text.foregroundChangedToken);
			if (text.originalForeground == wux::DependencyProperty::UnsetValue())
			{
				textBlock.ClearValue(foregroundProperty);
			}
			else
			{
				textBlock.SetValue(foregroundProperty, text.originalForeground);
			}
		}
	}

	info.texts.clear();
	info.textBrush = nullptr;
	info.overrideAllText = false;
}

void TaskbarAppearanceService::ProcessWaitCallback(void* parameter, BOOLEAN)
{
	static_cast<TaskbarAppearanceService *>(parameter)->OnProcessDied();
}

void TaskbarAppearanceService::EnsureSubclass(HWND taskbar)
{
	if (!m_SubclassedWindows.contains(taskbar))
	{
		if (SetWindowSubclass(taskbar, TaskbarSubclassProc, reinterpret_cast<UINT_PTR>(taskbar), reinterpret_cast<DWORD_PTR>(this)))
		{
			m_SubclassedWindows.insert(taskbar);
		}
	}
}

LRESULT TaskbarAppearanceService::TaskbarSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR dwRefData)
{
	switch (uMsg)
	{
	case WM_PAINT:
	{
		RECT rect;
		if (GetClientRect(hWnd, &rect))
		{
			if (wil::unique_hdc_window dc { wil::window_dc(GetDCEx(hWnd, nullptr, DCX_WINDOW | DCX_CACHE), hWnd) })
			{
				FillRect(dc.get(), &rect, reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
			}
		}

		break;
	}

	case WM_NCDESTROY:
		reinterpret_cast<TaskbarAppearanceService*>(dwRefData)->m_SubclassedWindows.erase(hWnd);
		break;
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}
