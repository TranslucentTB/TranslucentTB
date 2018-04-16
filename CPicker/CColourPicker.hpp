#pragma once
#include "../TranslucentTB/arch.h"
#include <cstdint>
#include <unordered_map>
#include <windef.h>

#include "SColour.hpp"

class CColourPicker
{
	public:
		_declspec(dllexport) CColourPicker(uint32_t &value, HWND hParentWindow = NULL);

		// Creates the colour picker dialog
		_declspec(dllexport) void CreateColourPicker();

		// Functions to set the colour components
		// NOTE: SetRGB automatically updates HSV and viceversa
		_declspec(dllexport) void SetRGB(uint8_t r, uint8_t g, uint8_t b);
		_declspec(dllexport) void SetHSV(unsigned short h, uint8_t s, uint8_t v);
		_declspec(dllexport) void SetAlpha(uint8_t a);
		
		// Some easy functions to retrieve the colour components
		_declspec(dllexport) const SColour &GetCurrentColour();
		_declspec(dllexport) const SColour &GetOldColour();

		_declspec(dllexport) void UpdateOldColour();

		friend int CALLBACK ColourPickerDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	private:
		void UpdateValue();

		static std::unordered_map<uint32_t *, HWND> PickerMap;
		uint32_t &Value;
		// The current selected colour and the previous selected one
		SColour CurrCol, OldCol;
		HWND hParent;
};
