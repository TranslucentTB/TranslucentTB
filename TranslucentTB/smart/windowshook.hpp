#pragma once
#include "arch.h"
#include <utility>
#include <WinBase.h>
#include <windef.h>
#include <WinUser.h>

#include "ttberror.hpp"

class WindowsHook {
private:
	HHOOK m_hook;

public:
	inline WindowsHook(HHOOK hook = nullptr) : m_hook(hook) { }

	inline WindowsHook(WindowsHook &&other) noexcept : m_hook(std::exchange(other.m_hook, nullptr))
	{
		return;
	}

	inline WindowsHook &operator =(WindowsHook &&other) noexcept
	{
		if (this != &other)
		{
			m_hook = std::exchange(other.m_hook, nullptr);
		}
		return *this;
	}

	inline WindowsHook(const WindowsHook &other) = delete;
	inline WindowsHook &operator =(const WindowsHook &other) = delete;

	inline ~WindowsHook()
	{
		if (m_hook && !UnhookWindowsHookEx(m_hook))
		{
			LastErrorHandle(Error::Level::Log, L"Failed to remove hook.");
		}
	}
};