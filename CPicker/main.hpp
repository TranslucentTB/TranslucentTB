#pragma once
#include "../TranslucentTB/arch.h"
#include <windef.h>

#include "SColour.hpp"

int CALLBACK ColourPickerDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
void UpdateValues(HWND hDlg, const SColour &col, bool &changing_text);