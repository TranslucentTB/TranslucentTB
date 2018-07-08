#pragma once
#include "../TranslucentTB/arch.h"
#include <atlbase.h>
#include <cstdint>
#include <unordered_map>
#include <windef.h>

#include "ccolourpicker.hpp"
#include "scolour.hpp"
#include "../TranslucentTB/util.hpp"

class GUI {
private:
	static const Util::string_map<uint32_t> COLOR_MAP;
	static const std::unordered_map<unsigned int, const std::pair<const unsigned int, const unsigned int>> SLIDER_MAP;
	static std::unordered_map<uint32_t *, HWND> m_pickerMap;

	static INT_PTR CALLBACK ColourPickerDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK NoOutlineButtonProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static void UpdateValues(HWND hDlg, const SColour &col, bool &changing_text);
	static void FailedParse(HWND hDlg);
	static void ParseHex(HWND hDlg, CColourPicker *picker);

	inline static uint8_t ExpandOneLetterByte(const uint8_t &byte)
	{
		const uint8_t firstDigit = byte & 0xF;
		return (firstDigit << 4) + firstDigit;
	}

public:
	static HRESULT CreateGUI(CColourPicker *picker, uint32_t &value, HWND hParent);
};