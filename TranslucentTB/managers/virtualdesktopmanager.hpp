#pragma once
#include <optional>
#include <string>
#include <vector>
#include <wrl/client.h>
#include "undoc/virtualdesktop.hpp"

struct VirtualDesktopInfo {
	GUID Id{};
	std::wstring Name;
	UINT Position = 0; // one-based display label, never a configuration key
};

// Own and use on the application's COM-initialized main thread. Notifications
// only post a message: no rendering, shell queries, or application callbacks
// execute inside Explorer's synchronous notification callback.
class VirtualDesktopManager final {
private:
	Microsoft::WRL::ComPtr<VirtualDesktopAbi::Manager> m_Manager;
	Microsoft::WRL::ComPtr<VirtualDesktopAbi::NotificationService> m_Service;
	Microsoft::WRL::ComPtr<VirtualDesktopAbi::Notification> m_Sink;
	std::optional<DWORD> m_Cookie;
	std::optional<VirtualDesktopInfo> m_Current;
	std::vector<VirtualDesktopInfo> m_Desktops;
	HRESULT m_LastError = S_OK;

public:
	VirtualDesktopManager() = default;
	VirtualDesktopManager(const VirtualDesktopManager &) = delete;
	VirtualDesktopManager &operator=(const VirtualDesktopManager &) = delete;
	~VirtualDesktopManager();

	void Disconnect() noexcept;
	HRESULT Connect(HWND window, UINT message);
	HRESULT Refresh();

	const std::optional<VirtualDesktopInfo> &Current() const noexcept { return m_Current; }
	const std::vector<VirtualDesktopInfo> &Desktops() const noexcept { return m_Desktops; }
	HRESULT LastError() const noexcept { return m_LastError; }
};
