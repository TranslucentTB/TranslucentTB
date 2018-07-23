#pragma once
#include "../TranslucentTB/arch.h"
#include <cstdint>
#include <d2d1_3.h>
#include <tuple>
#include <unordered_map>
#include <windef.h>
#include <WinUser.h>
#include <CommCtrl.h>

#include "alphaslidercontext.hpp"
#include "colorslidercontext.hpp"
#include "colorpreviewcontext.hpp"
#include "ccolourpicker.hpp"
#include "mainpickercontext.hpp"
#include "resource.h"
#include "scolour.hpp"
#include "../TranslucentTB/util.hpp"

class GUI {
private:
	static const Util::string_map<const uint32_t> COLOR_MAP;
	static const std::tuple<const unsigned int, const unsigned int, const unsigned int> SLIDERS[8];
	static std::unordered_map<const uint32_t *, HWND> m_pickerMap;

	static INT_PTR CALLBACK ColourPickerDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK NoOutlineButtonSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR);

	inline static void FailedParse(HWND hDlg)
	{
		EDITBALLOONTIP ebt = {
			sizeof(ebt),
			L"Error when parsing color code!",
			L"Make sure the code is valid hexadecimal. (0x and # prefixes accepted)\n"
			L"Code can be 3 (RGB), 4 (RGBA), 6 (RRGGBB) or 8 (RRGGBBAA) characters.\n\n"
			L"HTML color names are also understood. (for example: yellow, white, blue)",
			TTI_WARNING_LARGE
		};

		Edit_ShowBalloonTip(GetDlgItem(hDlg, IDC_HEXCOL), &ebt);
	}

	inline static uint8_t ExpandOneLetterByte(const uint8_t &byte)
	{
		const uint8_t firstDigit = byte & 0xF;
		return (firstDigit << 4) + firstDigit;
	}

	using initdialog_pair_t = const std::pair<GUI *const, const uint32_t *const>;

	CColourPicker *const m_picker;

	MainPickerContext m_pickerContext;
	ColorSliderContext m_colorSliderContext;
	AlphaSliderContext m_alphaSliderContext;
	ColorPreviewContext m_colorPreviewContext;
	const std::pair<RenderContext &, const unsigned int> m_contextPairs[4];

	bool m_changingText;
	bool m_changingHexViaSpin;
	HWND m_oldColorTip;

	GUI(CColourPicker *const picker, ID2D1Factory3 *const factory);

	void UpdateValues(HWND hDlg);
	void ParseHex(HWND hDlg);

	inline GUI(const GUI &) = delete;
	inline GUI &operator =(const GUI &) = delete;

public:
	static HRESULT CreateGUI(CColourPicker *picker, uint32_t &value, HWND hParent);
};