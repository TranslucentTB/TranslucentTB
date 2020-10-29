#pragma once
#include "basexamlpagehost.hpp"
#include <concepts>
#include <limits>
#include <ShObjIdl_core.h>
#include <string>
#include <type_traits>

#include "winrt.hpp"
#include "undefgetcurrenttime.h"
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.UI.Xaml.h>
#include <winrt/Windows.UI.Xaml.Hosting.h>
#include <winrt/Windows.System.h>
#include "redefgetcurrenttime.h"

#include "util/string_macros.hpp"
#include "../ProgramLog/error/win32.hpp"
#include "../tray/contextmenu.hpp"
#include "undoc/dynamicloader.hpp"

template<typename T>
requires std::derived_from<T, winrt::impl::base_one<T, winrt::TranslucentTB::Xaml::Pages::FramelessPage>> // https://github.com/microsoft/cppwinrt/issues/609
class XamlPageHost final : public BaseXamlPageHost {
private:
	using callback_t = std::add_pointer_t<void(void*)>;

	T m_content;
	winrt::Windows::System::DispatcherQueue m_Dispatcher;

	winrt::event_token m_AlwaysOnTopChangedToken;
	winrt::event_token m_TitleChangedToken;
	winrt::event_token m_IsClosableChangedToken;
	typename T::Closed_revoker m_ClosedRevoker;
	typename T::LayoutUpdated_revoker m_LayoutUpdatedRevoker;

	winrt::Windows::Foundation::EventHandler<winrt::Windows::Foundation::IInspectable> m_LayoutUpdatedHandler = { this, &XamlPageHost::OnXamlLayoutChanged };
	winrt::Windows::System::DispatcherQueueHandler m_SizeUpdater = { this, &XamlPageHost::UpdateWindowSize };

	bool m_PendingSizeUpdate = false;
	callback_t m_Callback;
	void *m_CallbackData;

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

	inline void InitialUpdateWindowSize(xaml_startup_position position)
	{
		m_LayoutUpdatedRevoker.revoke();
		const auto guard = wil::scope_exit([this]
		{
			m_PendingSizeUpdate = false;
			m_LayoutUpdatedRevoker = m_content.LayoutUpdated(winrt::auto_revoke, m_LayoutUpdatedHandler);
		});

		POINT cursor = { 0, 0 };
		const HMONITOR mon = GetInitialMonitor(cursor, position);

		MONITORINFO info = { sizeof(info) };
		const auto [width, height] = GetXamlSize(mon, info);

		if (const auto wndRect = rect())
		{
			int x = wndRect->left, y = wndRect->top;
			CalculateInitialPosition(x, y, width, height, cursor, info.rcWork, position);
			ResizeWindow(x, y, width, height, true, SWP_SHOWWINDOW);
			SetForegroundWindow(m_WindowHandle);
		}
	}

	// This method is in a fairly hot path, especially when using the color picker
	inline void UpdateWindowSize()
	{
		m_LayoutUpdatedRevoker.revoke();
		const auto guard = wil::scope_exit([this]
		{
			m_PendingSizeUpdate = false;
			if (m_content)
			{
				m_LayoutUpdatedRevoker = m_content.LayoutUpdated(winrt::auto_revoke, m_LayoutUpdatedHandler);
			}
		});

		if (m_content)
		{
			MONITORINFO info = { sizeof(info) };
			const auto [newWidth, newHeight] = GetXamlSize(monitor(), info);

			const auto wndRect = rect();
			if (wndRect && (wndRect->bottom - wndRect->top != newHeight || wndRect->right - wndRect->left != newWidth)) [[unlikely]]
			{
				int x = wndRect->left, y = wndRect->top;
				const bool move = AdjustWindowPosition(x, y, newWidth, newHeight, info.rcWork);
				ResizeWindow(x, y, newWidth, newHeight, move);
			}
		}
	}

	inline std::pair<int, int> GetXamlSize(HMONITOR mon, MONITORINFO &info)
	{
		if (!GetMonitorInfo(mon, &info)) [[unlikely]]
		{
			LastErrorHandle(spdlog::level::warn, L"Failed to get monitor info");
			return { 0, 0 };
		}

		const auto scale = GetDpiScale(mon);
		winrt::Windows::Foundation::Size size = {
			(info.rcWork.right - info.rcWork.left) / scale,
			(info.rcWork.bottom - info.rcWork.top) / scale
		};

		m_content.Measure(size);
		size = m_content.DesiredSize();
		m_content.UpdateLayout(); // prevent the call to Measure from firing a LayoutUpdated event

		return { static_cast<int>(size.Width * scale), static_cast<int>(size.Height * scale) };
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
		if (!SetWindowPos(m_WindowHandle, wnd, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE))
		{
			LastErrorHandle(spdlog::level::warn, L"Failed to change window topmost state");
		}
	}

	inline void UpdateIsClosable(const winrt::Windows::UI::Xaml::DependencyObject &, const winrt::Windows::UI::Xaml::DependencyProperty &)
	{
		ContextMenu::EnableItem(GetSystemMenu(m_WindowHandle, false), SC_CLOSE, m_content.IsClosable());
	}

	inline void UpdateTheme()
	{
		if (const auto saudm = DynamicLoader::ShouldAppsUseDarkMode())
		{
			// only the last bit has info, the rest is garbage
			using winrt::Windows::UI::Xaml::ElementTheme;
			m_content.RequestedTheme((saudm() & 0x1) ? ElementTheme::Dark : ElementTheme::Light);
		}
	}

	void OnClose()
	{
		// cleanup asap
		BeforeDelete();

		// dispatch the rest because cleaning up the XAML source here makes the XAML framework very angry
		m_Dispatcher.TryEnqueue(winrt::Windows::System::DispatcherQueuePriority::Low, [this]
		{
			BaseXamlPageHost::BeforeDelete();
			m_Callback(m_CallbackData);
		});
	}

	template<winrt::Windows::UI::Xaml::DependencyProperty(*prop)()>
	void UnregisterPropertyChangedCallback(winrt::event_token &token)
	{
		if (token)
		{
			m_content.UnregisterPropertyChangedCallback(prop(), token.value);
			token.value = 0;
		}
	}

	void BeforeDelete()
	{
		show(SW_HIDE);
		source().Content(nullptr);

		m_LayoutUpdatedRevoker.revoke();
		m_LayoutUpdatedHandler = nullptr;
		m_SizeUpdater = nullptr;
		m_ClosedRevoker.revoke();

		using winrt::TranslucentTB::Xaml::Pages::FramelessPage;
		UnregisterPropertyChangedCallback<FramelessPage::TitleProperty>(m_TitleChangedToken);
		UnregisterPropertyChangedCallback<FramelessPage::AlwaysOnTopProperty>(m_AlwaysOnTopChangedToken);
		UnregisterPropertyChangedCallback<FramelessPage::IsClosableProperty>(m_IsClosableChangedToken);

		m_content = nullptr;

		BaseXamlPageHost::Cleanup();
	}

public:
	template<typename... Args>
	inline XamlPageHost(HINSTANCE hInst, xaml_startup_position position, winrt::Windows::System::DispatcherQueue dispatcher, callback_t callback, void *data, Args&&... args) :
		BaseXamlPageHost(GetClassName(), hInst),
		m_content(std::forward<Args>(args)...),
		m_Dispatcher(std::move(dispatcher)),
		m_Callback(callback),
		m_CallbackData(data)
	{
		UpdateTheme();
		UpdateTitle(nullptr, nullptr);
		UpdateAlwaysOnTop(nullptr, nullptr);
		UpdateIsClosable(nullptr, nullptr);

		using winrt::TranslucentTB::Xaml::Pages::FramelessPage;
		m_IsClosableChangedToken.value = m_content.RegisterPropertyChangedCallback(FramelessPage::IsClosableProperty(), { this, &XamlPageHost::UpdateIsClosable });
		m_AlwaysOnTopChangedToken.value = m_content.RegisterPropertyChangedCallback(FramelessPage::AlwaysOnTopProperty(), { this, &XamlPageHost::UpdateAlwaysOnTop });
		m_TitleChangedToken.value = m_content.RegisterPropertyChangedCallback(FramelessPage::TitleProperty(), { this, &XamlPageHost::UpdateTitle });

		m_ClosedRevoker = m_content.Closed(winrt::auto_revoke, { this, &XamlPageHost::OnClose });
		m_LayoutUpdatedRevoker = m_content.LayoutUpdated(winrt::auto_revoke, [this, position](const winrt::Windows::Foundation::IInspectable &, const winrt::Windows::Foundation::IInspectable &)
		{
			if (!m_PendingSizeUpdate)
			{
				m_PendingSizeUpdate = m_Dispatcher.TryEnqueue(winrt::Windows::System::DispatcherQueuePriority::Low, [this, position]
				{
					InitialUpdateWindowSize(position);
				});
			}
		});

		source().Content(m_content);

		if (const auto initWithWnd = m_content.try_as<IInitializeWithWindow>())
		{
			HresultVerify(initWithWnd->Initialize(m_WindowHandle), spdlog::level::warn, L"Failed to initialize with window");
		}

		// TODO:
		// draggable titlebar
		// react to dpi change
		// tab navigation enabled on opening
	}

	inline winrt::TranslucentTB::Xaml::Pages::FramelessPage page() noexcept override
	{
		return m_content;
	}

	inline constexpr T &content() noexcept
	{
		return m_content;
	}
};
