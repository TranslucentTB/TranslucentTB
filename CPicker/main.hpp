#pragma once
#include "../TranslucentTB/arch.h"
#include <windef.h>

#include "SColour.hpp"

// Just some useful macros...
#define _IS_IN(min, max, x)  (((x)>(min)) && ((x)<(max)))
#define WIDTH(r) ((r).right-(r).left)
#define HEIGHT(r) ((r).bottom-(r).top)

int CALLBACK ColourPickerDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
void UpdateValues(HWND hDlg, const SColour &col, bool &changing_text);