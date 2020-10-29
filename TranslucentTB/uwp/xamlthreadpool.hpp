#pragma once
#include "arch.h"
#include <memory>
#include <utility>
#include <vector>
#include <windef.h>
#include "winrt.hpp"
#include <winrt/TranslucentTB.Xaml.h>

#include "../../ProgramLog/error/winrt.hpp"
#include "xamlpagehost.hpp"
#include "xamlthread.hpp"

class XamlThreadPool {
	HINSTANCE m_hInst;
	winrt::TranslucentTB::Xaml::App m_App = nullptr;

	std::vector<std::unique_ptr<XamlThread>> m_Threads;
	XamlThread &GetAvailableThread(std::unique_lock<Util::thread_independent_mutex> &lock);

public:
	XamlThreadPool(const XamlThreadPool &) = delete;
	XamlThreadPool &operator =(const XamlThreadPool &) = delete;

	inline XamlThreadPool(HINSTANCE hInst) noexcept : m_hInst(hInst) { }

	template<typename T, typename Callback, typename... Args>
	void CreateXamlWindow(xaml_startup_position pos, Callback &&callback, Args&&... args)
	{
		if (!m_App)
		{
			try
			{
				m_App = { };
			}
			HresultErrorCatch(spdlog::level::critical, L"Failed to create XAML application");
		}

		std::unique_lock<Util::thread_independent_mutex> guard;
		XamlThread &thread = GetAvailableThread(guard);
		thread.CreateXamlWindow<T>(std::move(guard), m_hInst, pos, std::forward<Callback>(callback), std::forward<Args>(args)...);
	}

	~XamlThreadPool();
};
