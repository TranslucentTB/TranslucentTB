#pragma once
#include "basexamlpagehost.hpp"
#include <concepts>
#include <ShObjIdl_core.h>
#include <tuple>
#include <type_traits>
#include <windowsx.h>

#include "winrt.hpp"
#include "undefgetcurrenttime.h"
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.UI.Xaml.h>
#include <winrt/Windows.UI.Xaml.Controls.h>
#include <winrt/Windows.UI.Xaml.Hosting.h>
#include <winrt/Windows.UI.Xaml.Media.h>
#include <winrt/Windows.System.h>
#include <winrt/TranslucentTB.Xaml.Pages.h>
#include "redefgetcurrenttime.h"
#include <wil/cppwinrt_helpers.h>

#include "util/string_macros.hpp"
#include "../ProgramLog/error/win32.hpp"

template<typename T>
class XamlPageHost final : public BaseXamlPageHost {
private:
	using callback_t = std::add_pointer_t<void(void*)>;

	T m_content;
	winrt::Windows::System::DispatcherQueue m_Dispatcher;

	winrt::event_token m_AlwaysOnTopChangedToken, m_TitleChangedToken, m_ClosedToken, m_LayoutUpdatedToken;

	bool m_PendingSizeUpdate = false;
	bool m_EndDragLayoutUpdate = false;
	callback_t m_Callback;
	void *m_CallbackData;

	inline LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam) override
	{
		switch (uMsg)
		{
		case WM_ACTIVATE:
			if (m_content)
			{
				m_content.IsActive(wParam != WA_INACTIVE);
			}
			break;

		case WM_SYSCOMMAND:
			if (wParam == SC_CLOSE)
			{
				TryClose();
				return 0;
			}
			break;

		case WM_PAINT:
		{
			if (const auto brush = m_content.Background().try_as<wux::Media::AcrylicBrush>())
			{
				PAINTSTRUCT ps;
				const auto paint = wil::BeginPaint(m_WindowHandle, &ps);
				if (paint && PaintBackground(ps.hdc, ps.rcPaint, brush.FallbackColor()))
				{
					return 0;
				}
			}
			break;
		}


		case WM_ERASEBKGND:
		{
			const auto rect = client_rect();
			const auto brush = m_content.Background().try_as<wux::Media::AcrylicBrush>();
			if (rect && brush && PaintBackground(reinterpret_cast<HDC>(wParam), *rect, brush.FallbackColor()))
			{
				return 1;
			}
			break;
		}

		case WM_CLOSE:
			TryClose();
			return 0;

		case WM_EXITSIZEMOVE:
			// some post-DPI change fixups if the size is not divisible by 4
			m_EndDragLayoutUpdate = true;
			m_content.InvalidateMeasure();
			m_content.InvalidateArrange();
			return 0;

		case WM_SYSKEYDOWN:
			if (wParam == VK_SPACE)
			{
				m_content.ShowSystemMenu({ 0, 0 });
				return 0;
			}
			break;

		case WM_NCLBUTTONDOWN:
			m_content.HideSystemMenu();
			if (wParam == HTCAPTION && !m_content.CanMove())
			{
				return 0;
			}
			break;

		case WM_NCRBUTTONUP:
			if (wParam == HTCAPTION)
			{
				if (const auto wndRect = rect())
				{
					const auto x = GET_X_LPARAM(lParam);
					const auto y = GET_Y_LPARAM(lParam);
					const auto scale = GetDpiScale(monitor());

					m_content.ShowSystemMenu({
						(x - wndRect->left) / scale,
						(y - wndRect->top) / scale
					});
					return 0;
				}
			}
			break;
		}

		return BaseXamlPageHost::MessageHandler(uMsg, wParam, lParam);
	}

	inline winrt::fire_and_forget InitialXamlLayoutChanged(xaml_startup_position position)
	{
		m_content.LayoutUpdated(m_LayoutUpdatedToken);
		m_LayoutUpdatedToken = { };
		co_await wil::resume_foreground(m_Dispatcher, winrt::Windows::System::DispatcherQueuePriority::Low);

		POINT cursor = { 0, 0 };
		const HMONITOR mon = GetInitialMonitor(cursor, position);

		MONITORINFO info = { sizeof(info) };
		const auto [windowSize, dragRegion, buttonRegion] = GetXamlSize(mon, info);

		int width = static_cast<int>(windowSize.Width),
			height = static_cast<int>(windowSize.Height),
			x = 0,
			y = 0;

		CalculateInitialPosition(x, y, width, height, cursor, info.rcWork, position);
		ResizeWindow(x, y, width, height, true, SWP_SHOWWINDOW);
		PositionDragRegion(dragRegion, buttonRegion, SWP_SHOWWINDOW);
		SetForegroundWindow(m_WindowHandle);

		m_LayoutUpdatedToken = m_content.LayoutUpdated({ this, &XamlPageHost::OnXamlLayoutChanged });
	}

	inline winrt::fire_and_forget OnXamlLayoutChanged(const wf::IInspectable &, const wf::IInspectable &)
	{
		if (!m_PendingSizeUpdate)
		{
			m_PendingSizeUpdate = true;
			const auto guard = wil::scope_exit([this]() noexcept
			{
				m_EndDragLayoutUpdate = false;
				m_PendingSizeUpdate = false;
			});

			co_await wil::resume_foreground(m_Dispatcher, winrt::Windows::System::DispatcherQueuePriority::Low);

			if (m_content)
			{
				MONITORINFO info = { sizeof(info) };
				const auto [windowSize, dragRegion, buttonRegion] = GetXamlSize(monitor(), info);

				const auto newHeight = static_cast<int>(windowSize.Height);
				const auto newWidth = static_cast<int>(windowSize.Width);
				const auto wndRect = rect();
				if (wndRect && (m_EndDragLayoutUpdate || wndRect->bottom - wndRect->top != newHeight || wndRect->right - wndRect->left != newWidth)) [[unlikely]]
				{
					bool move = true;
					int x = wndRect->left, y = wndRect->top;
					if (!m_EndDragLayoutUpdate)
					{
						move = AdjustWindowPosition(x, y, newWidth, newHeight, info.rcWork);
					}
					ResizeWindow(x, y, newWidth, newHeight, move);
				}

				PositionDragRegion(dragRegion, buttonRegion);
			}
		}
	}

	inline std::tuple<wf::Size, wf::Rect, wf::Rect> GetXamlSize(HMONITOR mon, MONITORINFO &info)
	{
		if (!GetMonitorInfo(mon, &info)) [[unlikely]]
		{
			LastErrorHandle(spdlog::level::warn, L"Failed to get monitor info");
			return { };
		}

		const auto scale = GetDpiScale(mon);
		wf::Size size = {
			(info.rcWork.right - info.rcWork.left) / scale,
			(info.rcWork.bottom - info.rcWork.top) / scale
		};

		m_content.Measure(size);
		size = m_content.DesiredSize();
		m_content.UpdateLayout(); // prevent the call to Measure from firing a LayoutUpdated event

		return {
			{
				size.Width * scale,
				size.Height * scale
			},
			ScaleRect(m_content.DragRegion(), scale),
			ScaleRect(m_content.TitlebarButtonsRegion(), scale)
		};
	}

	inline void UpdateTitle(const wux::DependencyObject &, const wux::DependencyProperty &)
	{
		if (!SetWindowText(m_WindowHandle, m_content.Title().c_str()))
		{
			LastErrorHandle(spdlog::level::warn, L"Failed to set window title");
		}
	}

	inline void UpdateAlwaysOnTop(const wux::DependencyObject &, const wux::DependencyProperty &)
	{
		const auto wnd = m_content.AlwaysOnTop() ? Window::TopMostWindow : Window::NoTopMostWindow;
		if (!SetWindowPos(m_WindowHandle, wnd, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE))
		{
			LastErrorHandle(spdlog::level::warn, L"Failed to change window topmost state");
		}
	}

	winrt::fire_and_forget OnClose()
	{
		Cleanup();

		// dispatch the deletion because cleaning up the XAML source here makes the XAML framework very angry
		co_await wil::resume_foreground(m_Dispatcher, winrt::Windows::System::DispatcherQueuePriority::Low);

		BaseXamlPageHost::Cleanup();
		if (m_Callback)
		{
			m_Callback(m_CallbackData);
		}
	}

	void Cleanup()
	{
		if (m_content)
		{
			show(SW_HIDE);
			if (m_LayoutUpdatedToken)
			{
				m_content.LayoutUpdated(m_LayoutUpdatedToken);
				m_LayoutUpdatedToken = { };
			}

			source().Content(nullptr);

			m_content.Closed(m_ClosedToken);

			using winrt::TranslucentTB::Xaml::Pages::FramelessPage;
			m_content.UnregisterPropertyChangedCallback(FramelessPage::TitleProperty(), m_TitleChangedToken.value);
			m_content.UnregisterPropertyChangedCallback(FramelessPage::AlwaysOnTopProperty(), m_AlwaysOnTopChangedToken.value);

			m_content = nullptr;
		}
	}

public:
	inline ~XamlPageHost()
	{
		Cleanup();
	}

	template<typename... Args>
	inline XamlPageHost(WindowClass &classRef, WindowClass &dragRegionClass, xaml_startup_position position,
		winrt::Windows::System::DispatcherQueue dispatcher, callback_t callback,
		void *data, Args&&... args) :
		BaseXamlPageHost(classRef, dragRegionClass),
		m_content(std::forward<Args>(args)...),
		m_Dispatcher(std::move(dispatcher)),
		m_Callback(callback),
		m_CallbackData(data)
	{
		UpdateTitle(nullptr, nullptr);
		UpdateAlwaysOnTop(nullptr, nullptr);

		using winrt::TranslucentTB::Xaml::Pages::FramelessPage;
		m_AlwaysOnTopChangedToken.value = m_content.RegisterPropertyChangedCallback(FramelessPage::AlwaysOnTopProperty(), { this, &XamlPageHost::UpdateAlwaysOnTop });
		m_TitleChangedToken.value = m_content.RegisterPropertyChangedCallback(FramelessPage::TitleProperty(), { this, &XamlPageHost::UpdateTitle });

		m_ClosedToken = m_content.Closed({ this, &XamlPageHost::OnClose });
		m_LayoutUpdatedToken = m_content.LayoutUpdated([this, position](const wf::IInspectable &, const wf::IInspectable &)
		{
			InitialXamlLayoutChanged(position);
		});

		source().Content(m_content);

		if (const auto initWithWnd = m_content.try_as<IInitializeWithWindow>())
		{
			HresultVerify(initWithWnd->Initialize(m_WindowHandle), spdlog::level::warn, L"Failed to initialize with window");
		}

		// TODO: no animation when closing through alt-space menu with keyboard
	}

	inline winrt::TranslucentTB::Xaml::Pages::FramelessPage page() noexcept override
	{
		return m_content;
	}

	inline constexpr const T &content() noexcept
	{
		return m_content;
	}

	bool TryClose() override
	{
		if (!m_content || m_content.RequestClose())
		{
			return true;
		}
		else
		{
			SetForegroundWindow(m_WindowHandle);
			return false;
		}
	}
};
