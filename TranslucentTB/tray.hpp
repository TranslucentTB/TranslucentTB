#pragma once
#include <cstdint>
#include <unordered_map>
#include <WinUser.h>

#include "resource.h"
#include "swcadata.hpp"
#include "taskbar.hpp"

namespace Tray {
	using namespace swca;
	using namespace Taskbar;

	static const wchar_t WM_TASKBARCREATED[] = L"TaskbarCreated";
	static const wchar_t NEW_TTB_INSTANCE[]  = L"NewTTBInstance";

	enum EXITREASON {
		NewInstance,		// New instance told us to exit
		UserAction,			// Triggered by the user
		UserActionNoSave	// Triggered by the user, but doesn't saves config
	};

	static const std::unordered_map<ACCENT, uint32_t> NORMAL_BUTTON_MAP = {
		{ ACCENT_NORMAL,						IDM_NORMAL },
		{ ACCENT_ENABLE_TRANSPARENTGRADIENT,	IDM_CLEAR },
		{ ACCENT_ENABLE_GRADIENT,				IDM_OPAQUE },
		{ ACCENT_ENABLE_BLURBEHIND,				IDM_BLUR },
		{ ACCENT_ENABLE_FLUENT,					IDM_FLUENT }
	};
	static const std::unordered_map<ACCENT, uint32_t> DYNAMIC_BUTTON_MAP = {

		{ ACCENT_NORMAL,						IDM_DYNAMICWS_NORMAL },
		{ ACCENT_ENABLE_TRANSPARENTGRADIENT,	IDM_DYNAMICWS_CLEAR },
		{ ACCENT_ENABLE_GRADIENT,				IDM_DYNAMICWS_OPAQUE },
		{ ACCENT_ENABLE_BLURBEHIND,				IDM_DYNAMICWS_BLUR },
		{ ACCENT_ENABLE_FLUENT,					IDM_DYNAMICWS_FLUENT },
	};
	static const std::unordered_map<AEROPEEK, uint32_t> PEEK_BUTTON_MAP = {
		{ AEROPEEK::Enabled,		IDM_PEEK },
		{ AEROPEEK::Dynamic,		IDM_DPEEK },
		{ AEROPEEK::Disabled,		IDM_NOPEEK }
	};

}