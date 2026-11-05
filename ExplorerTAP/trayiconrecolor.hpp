#pragma once
#include "arch.h"
#include <chrono>
#include <cstdint>
#include <deque>
#include <utility>
#include <windef.h>

// Recolors monochrome notification area icons sent by third party apps,
// so they match a custom taskbar text color without changing the system theme.
// Apps pick their tray icon color from the system theme, which we don't touch,
// so we intercept the icons as they get sent to the taskbar and remap them.
class TrayIconRecolor
{
public:
	TrayIconRecolor() noexcept = default;

	TrayIconRecolor(const TrayIconRecolor&) = delete;
	TrayIconRecolor& operator=(const TrayIconRecolor&) = delete;

	// color is ABGR, an alpha of 0 disables recoloring.
	// returns false if the hook could not be installed.
	// skipProcess is a process that shouldn't be asked to resend its icons (TranslucentTB itself).
	bool SetColor(uint32_t color, DWORD skipProcess);

	~TrayIconRecolor();

private:
	static LRESULT CALLBACK SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) noexcept;

	LRESULT OnCopyData(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept;
	HICON Recolor(HICON icon) const noexcept;
	void ReleaseOldIcons(bool all) noexcept;

	// asks every app to send its icons again, so they get (un)recolored.
	void RequestIconRefresh(DWORD skipProcess) noexcept;

	HWND m_Tray = nullptr;
	uint32_t m_Color = 0;
	std::chrono::steady_clock::time_point m_RefreshDeadline;

	// explorer copies the icon when it gets it, but keep ours alive
	// for a little while just in case it ever starts doing so asynchronously.
	std::deque<std::pair<std::chrono::steady_clock::time_point, HICON>> m_OldIcons;
};
