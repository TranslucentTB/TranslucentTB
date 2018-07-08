#pragma once
#include "../TranslucentTB/arch.h"
#include <cstdint>
#include <windef.h>

#include "ccolourpicker.hpp"
#include "rendercontext.hpp"

struct PickerData {
	CColourPicker *const picker;
	const std::pair<RenderContext *const, const unsigned int> contexts[4];
	const uint32_t *const value_ptr;

	bool changing_text;
	bool changing_hex_via_spin;
	HWND old_color_tip;
	WNDPROC button_proc;
};