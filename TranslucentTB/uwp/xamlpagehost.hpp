#pragma once
#include "../windows/messagewindow.hpp"

#include <string>
#include <type_traits>
#include <winrt/Windows.UI.Xaml.h>
#include <winrt/Windows.UI.Xaml.Hosting.h>
#include <winrt/Windows.UI.Xaml.Media.h>
#include <windows.ui.xaml.hosting.desktopwindowxamlsource.h>

namespace xaml = winrt::Windows::UI::Xaml;

template<typename T>
class XamlPageHost : public MessageWindow {
private:
	xaml::Hosting::WindowsXamlManager m_manager;
	xaml::Hosting::DesktopWindowXamlSource m_source;
	xaml::Media::ScaleTransform m_scaler;
	T m_content;

	Window m_interopWnd;

public:
	//static_assert(std::is_base_of_v<xaml::UIElement, T>, "T must be a XAML class");

	// TODO: support multiple instances
	template<typename... Args>
	inline XamlPageHost(const std::wstring &windowName, HINSTANCE hInst, Args&&... args) :
		MessageWindow(windowName, windowName, hInst, WS_OVERLAPPEDWINDOW),
		m_manager(xaml::Hosting::WindowsXamlManager::InitializeForCurrentThread()),
		// Don't construct the XAML stuff already.
		m_scaler(nullptr),
		m_content(nullptr)
	{
		auto nativeSource = m_source.as<IDesktopWindowXamlSourceNative>();
		winrt::check_hresult(nativeSource->AttachToWindow(m_WindowHandle));

		winrt::check_hresult(nativeSource->get_WindowHandle(m_interopWnd.put()));

		m_scaler = { };
		m_content = T(std::forward<Args>(args)...);

		m_content.RenderTransform(m_scaler);

		SetWindowPos(m_WindowHandle, 0, 0, 0, 300, 300, SWP_SHOWWINDOW);
		SetWindowPos(m_interopWnd, 0, 0, 0, 300, 300, SWP_SHOWWINDOW);
		show();
		UpdateWindow(m_WindowHandle);
		SetFocus(m_WindowHandle);
	}

	~XamlPageHost()
	{
		m_scaler = nullptr;
		m_content = nullptr;

		m_source.Close();
		m_source = nullptr;

		m_manager.Close();
		m_manager = nullptr;
	}

	XamlPageHost(const XamlPageHost &) = delete;
	XamlPageHost &operator =(const XamlPageHost &) = delete;
};