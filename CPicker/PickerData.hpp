#pragma once
#include "../TranslucentTB/arch.h"
#include <array>
#include <windef.h>

#include "CColourPicker.hpp"
#include "rendercontext.hpp"

struct PickerData {
	CColourPicker *picker;
	// TODO: change std::array size
	const std::array<const std::pair<RenderContext *const, const unsigned int>, 1> contexts;

	bool changing_text;
	bool changing_hex_via_spin;
	HWND old_color_tip;
	WNDPROC button_proc;
};