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
#include "ccolourpicker.hpp"
#include "mainpickercontext.hpp"
#include "newpreviewcontext.hpp"
#include "oldpreviewcontext.hpp"
#include "resource.h"
#include "scolour.hpp"
#include "../TranslucentTB/util.hpp"

class GUI {
private:
	static const Util::string_view_map<const uint32_t> COLOR_MAP;
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

	static constexpr uint8_t ExpandOneLetterByte(uint8_t byte)
	{
		const uint8_t firstDigit = byte & 0xF;
		return (firstDigit << 4) + firstDigit;
	}

	using initdialog_pair_t = const std::pair<GUI *const, const uint32_t *const>;

	CColourPicker *m_picker;

	MainPickerContext m_pickerContext;
	ColorSliderContext m_colorSliderContext;
	AlphaSliderContext m_alphaSliderContext;
	OldPreviewContext m_oldPreviewContext;
	NewPreviewContext m_newPreviewContext;
	const std::pair<RenderContext &, const unsigned int> m_contextPairs[5];

	bool m_changingText;
	bool m_changingHexViaSpin;
	HWND m_oldColorTip;
	HWND m_newColorTip;

	GUI(CColourPicker *picker, ID2D1Factory3 *factory, IDWriteFactory *dwFactory);

	INT_PTR OnDialogInit(HWND hDlg);
	INT_PTR OnDpiChange(HWND hDlg);
	INT_PTR OnPaint(HWND hDlg);
	INT_PTR OnClick(HWND hDlg, LPARAM lParam);
	void OnColorPickerClick(HWND hDlg, RECT position, POINT cursor);
	void OnColorSliderClick(HWND hDlg, RECT position, POINT cursor);
	void OnAlphaSliderClick(HWND hDlg, RECT position, POINT cursor);
	INT_PTR OnMouseMove(HWND hDlg, WPARAM wParam);
	INT_PTR OnCommand(HWND hDlg, WPARAM wParam);
	INT_PTR OnEditControlFocusAcquire(HWND hDlg, WPARAM wParam);
	INT_PTR OnEditControlFocusLoss(HWND hDlg, WPARAM wParam);
	INT_PTR OnEditControlTextChange(HWND hDlg, WPARAM wParam);
	INT_PTR OnButtonClick(HWND hDlg, WPARAM wParam);
	INT_PTR OnNotify(HWND hDlg, LPARAM lParam);
	INT_PTR OnUpDownControlChange(NMHDR notify);
	INT_PTR OnEditControlRequestWatermarkInfo(HWND hDlg, NMHDR &notify);
	INT_PTR OnNonClientCalculateSize(HWND hDlg, WPARAM wParam);
	INT_PTR OnWindowDestroy();

	HRESULT DrawItem(HWND hDlg, RenderContext &context, unsigned int id, const SColourF &col, const SColourF &old);

	void UpdateValues(HWND hDlg);
	void ParseHex(HWND hDlg);

	inline GUI(const GUI &) = delete;
	inline GUI &operator =(const GUI &) = delete;

public:
	static HRESULT CreateGUI(CColourPicker *picker, uint32_t &value, HWND hParent);
};