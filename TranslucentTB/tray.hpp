#pragma once
#ifndef TRAY_HPP
#define TRAY_HPP

#include <cstdint>
#include <unordered_map>

#include "swcadata.hpp"
#include "taskbar.hpp"

namespace Tray {
	using namespace swca;
	using namespace Taskbar;

	static const uint32_t WM_NOTIFY_TB = 3141;

	static const uint32_t WM_TASKBARCREATED = RegisterWindowMessage(L"TaskbarCreated");
	static const uint32_t NEW_TTB_INSTANCE = RegisterWindowMessage(L"NewTTBInstance");

	enum EXITREASON {
		NewInstance,		// New instance told us to exit
		UserAction,			// Triggered by the user
		UserActionNoSave	// Triggered by the user, but doesn't saves config
	};

	static const std::unordered_map<ACCENTSTATE, uint32_t> NORMAL_BUTTON_MAP = {
		{ ACCENT_ENABLE_BLURBEHIND,				IDM_BLUR },
		{ ACCENT_ENABLE_TRANSPARENTGRADIENT,	IDM_CLEAR },
		{ ACCENT_NORMAL,						IDM_NORMAL },
		{ ACCENT_ENABLE_GRADIENT,				IDM_OPAQUE },
		{ ACCENT_ENABLE_FLUENT,					IDM_FLUENT }
	};
	static const std::unordered_map<ACCENTSTATE, uint32_t> DYNAMIC_BUTTON_MAP = {
		{ ACCENT_ENABLE_BLURBEHIND,				IDM_DYNAMICWS_BLUR },
		{ ACCENT_ENABLE_TINTED,					IDM_DYNAMICWS_CLEAR },
		{ ACCENT_NORMAL,						IDM_DYNAMICWS_NORMAL },
		{ ACCENT_ENABLE_GRADIENT,				IDM_DYNAMICWS_OPAQUE },
		{ ACCENT_ENABLE_FLUENT,					IDM_DYNAMICWS_FLUENT }
	};
	static const std::unordered_map<AEROPEEKSTATE, uint32_t> PEEK_BUTTON_MAP = {
		{ Disabled,	    IDM_NOPEEK },
		{ Dynamic,		IDM_DPEEK },
		{ Enabled,		IDM_PEEK }
	};

}

#endif // !TRAY_HPP
