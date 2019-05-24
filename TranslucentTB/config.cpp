#include "config.hpp"
#include <iomanip>
#include <sstream>
#include <wingdi.h>

std::wstring_view TaskbarAppearance::AccentToString() const
{
	switch (Accent)
	{
	case ACCENT_NORMAL: return L"normal";
	case ACCENT_ENABLE_GRADIENT: return L"opaque";
	case ACCENT_ENABLE_TRANSPARENTGRADIENT: return L"clear";
	case ACCENT_ENABLE_BLURBEHIND: return L"blur";
	case ACCENT_ENABLE_ACRYLICBLURBEHIND: return L"acrylic";
	default: return { };
	}
}

std::wstring TaskbarAppearance::ColorToString() const
{
	std::wstringstream ss;
	ss << L'#' << std::uppercase << std::setfill(L'0') << std::hex;
	ss << std::setw(2) << GetRValue(Color);
	ss << std::setw(2) << GetGValue(Color);
	ss << std::setw(2) << GetBValue(Color);
	return ss.str();
}

std::wstring_view Config::PeekToString() const
{
	switch (Peek)
	{
	case PeekBehavior::AlwaysHide: return L"never";
	case PeekBehavior::WindowMaximisedOnMainMonitor: return L"when_maximised_window_on_main_monitor";
	case PeekBehavior::WindowMaximisedOnAnyMonitor: return L"when_maximised_window_on_any_monitor";
	case PeekBehavior::DesktopIsForegroundWindow: return L"when_desktop_is_foreground_window";
	case PeekBehavior::AlwaysShow: return L"always";
	default: return { };
	}
}
