#pragma once
#include "../TranslucentTB/arch.h"
#include <cstdint>
#include <windef.h>

#include "CColourPicker.hpp"
#include "SColour.hpp"

LRESULT CALLBACK NoOutlineButtonProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
uint8_t ExpandOneLetterByte(const uint8_t &byte);
void UpdateValues(HWND hDlg, const SColour &col, bool &changing_text);
void FailedParse(HWND hDlg);
void ParseHex(HWND hDlg, CColourPicker *picker);