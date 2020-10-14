#pragma once
#include "basexamlpagehost.hpp"
#include <concepts>
#include <limits>
#include <ShObjIdl_core.h>
#include <string>

#include "winrt.hpp"
#include "undefgetcurrenttime.h"
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.UI.Xaml.h>
#include <winrt/Windows.UI.Xaml.Hosting.h>
#include <winrt/Windows.System.h>
#include <winrt/TranslucentTB.Xaml.Pages.h>
#include "redefgetcurrenttime.h"

#include "util/string_macros.hpp"
#include "../ProgramLog/error/win32.hpp"
#include "undoc/dynamicloader.hpp"

template<typename T>
requires std::derived_from<T, winrt::impl::base_one<T, winrt::TranslucentTB::Xaml::Pages::FramelessPage>> // https://github.com/microsoft/cppwinrt/issues/609
class XamlPageHost final : public BaseXamlPageHost {
private:
	T m_content;
	winrt::Windows::System::DispatcherQueue m_Dispatcher;

	winrt::event_token m_AlwaysOnTopChangedToken;
	winrt::event_token m_TitleChangedToken;
	typename T::LayoutUpdated_revoker m_LayoutUpdatedRevoker;
	typename T::Closed_revoker m_ClosedRevoker;

	winrt::Windows::Foundation::EventHandler<winrt::Windows::Foundation::IInspectable> m_LayoutUpdatedHandler = { this, &XamlPageHost::OnXamlLayoutChanged };
	winrt::Windows::System::DispatcherQueueHandler m_SizeUpdater = { this, &XamlPageHost::UpdateWindowSize };

	bool m_PendingSizeUpdate = false;
	bool m_Initial = true;
	const xaml_startup_position m_Position;

	static constexpr std::wstring_view ExtractTypename()
	{
		constexpr std::wstring_view prefix = L"XamlPageHost<struct winrt::";
		constexpr std::wstring_view suffix = L">::ExtractTypename";

		std::wstring_view funcsig = UTIL_WIDEN(__FUNCSIG__);
		funcsig.remove_prefix(funcsig.find(prefix) + prefix.length());
		funcsig.remove_suffix(funcsig.length() - funcsig.rfind(suffix));

		return funcsig;
	}

	inline static const std::wstring &GetClassName()
	{
		static constexpr std::wstring_view type_name = ExtractTypename();
		static const std::wstring class_name(type_name);

		return class_name;
	}

	inline LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam) override
	{
		switch (uMsg)
		{
		case WM_SYSCOMMAND:
			if (wParam == SC_CLOSE)
			{
				if (!m_content.RequestClose())
				{
					Flash();
				}

				return 0;
			}
			break;

		case WM_SETTINGCHANGE:
			UpdateTheme();
			break;

		case WM_CLOSE:
			if (!m_content.RequestClose())
			{
				Flash();
			}

			return 0;
		}

		return BaseXamlPageHost::MessageHandler(uMsg, wParam, lParam);
	}

	inline void OnXamlLayoutChanged(const winrt::Windows::Foundation::IInspectable &, const winrt::Windows::Foundation::IInspectable &)
	{
		if (!m_PendingSizeUpdate)
		{
			m_PendingSizeUpdate = m_Dispatcher.TryEnqueue(winrt::Windows::System::DispatcherQueuePriority::Low, m_SizeUpdater);
		}
	}

	inline void UpdateWindowSize()
	{
		m_LayoutUpdatedRevoker.revoke();
		const auto guard = wil::scope_exit([this]
		{
			m_PendingSizeUpdate = false;
			m_LayoutUpdatedRevoker = m_content.LayoutUpdated(winrt::auto_revoke, m_LayoutUpdatedHandler);
		});

		POINT cursor = { 0, 0 };
		const HMONITOR mon = !m_Initial ? monitor() : GetInitialMonitor(cursor, m_Position);
		MONITORINFO info = { sizeof(info) };
		if (!GetMonitorInfo(mon, &info))
		{
			LastErrorHandle(spdlog::level::warn, L"Failed to get monitor info");
			return;
		}

		const auto scale = GetDpiScale(mon);
		winrt::Windows::Foundation::Size size = {
			(info.rcWork.right - info.rcWork.left) * scale,
			(info.rcWork.bottom - info.rcWork.top) * scale
		};

		m_content.Measure(size);
		size = m_content.DesiredSize();
		m_content.UpdateLayout(); // prevent the call to Measure from firing a LayoutUpdated event

		const auto newHeight = static_cast<int>(size.Height * scale);
		const auto newWidth = static_cast<int>(size.Width * scale);

		const auto wndRect = rect();
		if (wndRect && (wndRect->bottom - wndRect->top != newHeight || wndRect->right - wndRect->left != newWidth || m_Initial))
		{
			int x = wndRect->left, y = wndRect->top;
			bool move = false;
			if (!m_Initial)
			{
				move = AdjustWindowPosition(x, y, newWidth, newHeight, info.rcWork);
			}
			else
			{
				CalculateInitialPosition(x, y, newWidth, newHeight, cursor, info.rcWork, m_Position);
				move = true;
			}

			ResizeWindow(x, y, newWidth, newHeight, move);

			if (m_Initial)
			{
				show();
				if (!SetForegroundWindow(m_WindowHandle))
				{
					LastErrorHandle(spdlog::level::warn, L"Failed to set foreground window");
				}
				m_Initial = false;
			}
		}
	}

	inline void UpdateTitle(const winrt::Windows::UI::Xaml::DependencyObject &, const winrt::Windows::UI::Xaml::DependencyProperty &)
	{
		if (!SetWindowText(m_WindowHandle, m_content.Title().c_str()))
		{
			LastErrorHandle(spdlog::level::warn, L"Failed to set window title");
		}
	}

	inline void UpdateAlwaysOnTop(const winrt::Windows::UI::Xaml::DependencyObject &, const winrt::Windows::UI::Xaml::DependencyProperty &)
	{
		const auto wnd = m_content.AlwaysOnTop() ? Window::TopMostWindow : Window::NoTopMostWindow;
		if (!SetWindowPos(m_WindowHandle, wnd, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE))
		{
			LastErrorHandle(spdlog::level::warn, L"Failed to change window topmost state");
		}
	}

	inline void UpdateTheme()
	{
		if (DynamicLoader::uxtheme())
		{
			if (const auto saudm = DynamicLoader::ShouldAppsUseDarkMode())
			{
				// only the last bit has info, the rest is garbage
				using winrt::Windows::UI::Xaml::ElementTheme;
				m_content.RequestedTheme((saudm() & 0x1) ? ElementTheme::Dark : ElementTheme::Light);
			}
		}
	}

	void OnClose()
	{
		// hide the window asap to give the impression of responsiveness
		show(SW_HIDE);

		// dispatch the deletion because deleting things here makes the XAML framework very angry
		m_Dispatcher.TryEnqueue(winrt::Windows::System::DispatcherQueuePriority::Low, [this]
		{
			delete this;
		});
	}

	inline ~XamlPageHost()
	{
		source().Content(nullptr);
		m_ClosedRevoker.revoke();
		m_LayoutUpdatedRevoker.revoke();
		m_LayoutUpdatedHandler = nullptr;
		m_content.UnregisterPropertyChangedCallback(winrt::TranslucentTB::Xaml::Pages::FramelessPage::TitleProperty(), std::exchange(m_TitleChangedToken.value, 0));
		m_content.UnregisterPropertyChangedCallback(winrt::TranslucentTB::Xaml::Pages::FramelessPage::AlwaysOnTopProperty(), std::exchange(m_AlwaysOnTopChangedToken.value, 0));
		m_content = nullptr;
	}

public:
	template<typename... Args>
	inline XamlPageHost(HINSTANCE hInst, xaml_startup_position position, winrt::Windows::System::DispatcherQueue dispatcher, Args&&... args) :
		BaseXamlPageHost(GetClassName(), hInst),
		m_content(std::forward<Args>(args)...),
		m_Dispatcher(std::move(dispatcher)),
		m_Position(position)
	{
		UpdateTheme();
		UpdateTitle(nullptr, nullptr);
		UpdateAlwaysOnTop(nullptr, nullptr);

		m_AlwaysOnTopChangedToken.value = m_content.RegisterPropertyChangedCallback(winrt::TranslucentTB::Xaml::Pages::FramelessPage::AlwaysOnTopProperty(), { this, &XamlPageHost::UpdateAlwaysOnTop });
		m_TitleChangedToken.value = m_content.RegisterPropertyChangedCallback(winrt::TranslucentTB::Xaml::Pages::FramelessPage::TitleProperty(), { this, &XamlPageHost::UpdateTitle });
		m_LayoutUpdatedRevoker = m_content.LayoutUpdated(winrt::auto_revoke, m_LayoutUpdatedHandler);
		m_ClosedRevoker = m_content.Closed(winrt::auto_revoke, { this, &XamlPageHost::OnClose });

		source().Content(m_content);

		if (const auto initWithWnd = m_content.try_as<IInitializeWithWindow>())
		{
			HresultVerify(initWithWnd->Initialize(m_WindowHandle), spdlog::level::warn, L"Failed to initialize with window");
		}

		// TODO:
		// draggable titlebar
		// focus window on open
		// keyboard focus issues (setfocus?)
		// react to dpi change
		// not acrylic on first open
		// the window shows up under the tray overflow
	}


	inline constexpr T &content() noexcept
	{
		return m_content;
	}
};
