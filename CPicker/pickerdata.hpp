#pragma once
#include "../TranslucentTB/arch.h"
#include <array>
#include <cstdint>
#include <windef.h>

#include "ccolourpicker.hpp"
#include "rendercontext.hpp"

struct PickerData {
	CColourPicker *picker;
	const std::array<const std::pair<RenderContext *const, const unsigned int>, 4> contexts;
	uint32_t *value_ptr;

	bool changing_text;
	bool changing_hex_via_spin;
	HWND old_color_tip;
	WNDPROC button_proc;
};