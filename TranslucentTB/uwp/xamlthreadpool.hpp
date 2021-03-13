#pragma once
#include "arch.h"
#include <memory>
#include <utility>
#include <vector>
#include <windef.h>

#include "../../ProgramLog/error/winrt.hpp"
#include "undoc/uxtheme.hpp"
#include "../windows/windowclass.hpp"
#include "xamlpagehost.hpp"
#include "xamlthread.hpp"

class XamlThreadPool {
	WindowClass m_WndClass;
	WindowClass m_DragRegionClass;

	std::vector<std::unique_ptr<XamlThread>> m_Threads;
	XamlThread &GetAvailableThread(std::unique_lock<Util::thread_independent_mutex> &lock);

public:
	XamlThreadPool(const XamlThreadPool &) = delete;
	XamlThreadPool &operator =(const XamlThreadPool &) = delete;

	inline XamlThreadPool(HINSTANCE hInst) :
		m_WndClass(MessageWindow::MakeWindowClass(L"XamlPageHost", hInst)),
		m_DragRegionClass(MessageWindow::MakeWindowClass(L"XamlDragRegion", hInst))
	{ }

	template<typename T, typename Callback, typename... Args>
	void CreateXamlWindow(xaml_startup_position pos, Callback &&callback, Args&&... args)
	{
		std::unique_lock<Util::thread_independent_mutex> guard;
		XamlThread &thread = GetAvailableThread(guard);
		thread.CreateXamlWindow<T>(std::move(guard), m_WndClass, m_DragRegionClass, pos, std::forward<Callback>(callback), std::forward<Args>(args)...);
	}

	~XamlThreadPool();
};
