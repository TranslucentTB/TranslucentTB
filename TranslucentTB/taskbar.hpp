#pragma once
#include "arch.h"
#include <windef.h>

namespace Taskbar {

	enum TASKBARSTATE {
		Normal,				// If no dynamic options are set, act as it says in opt.taskbar_appearance
		WindowMaximised,	// There is a window which is maximised on the monitor this HWND is in. Display as blurred.
		StartMenuOpen		// The Start Menu is open on the monitor this HWND is in. Display as it would be without TranslucentTB active.
	};

	enum class AEROPEEK {
		Disabled,		// Hide the button
		Dynamic,		// Show when a window is maximised
		Enabled			// Don't hide the button
	};

	struct TASKBARPROPERTIES
	{
		HWND hwnd;
		TASKBARSTATE state;
	};

}