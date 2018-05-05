#pragma once
#include "../TranslucentTB/arch.h"
#include <cstdint>
#include <windef.h>

#include "SColour.hpp"

int CALLBACK ColourPickerDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
uint8_t ExpandOneLetterByte(const uint8_t &byte);
void UpdateValues(HWND hDlg, const SColour &col, bool &changing_text);