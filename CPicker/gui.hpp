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
#include "dlldata.hpp"
#include "mainpickercontext.hpp"
#include "newpreviewcontext.hpp"
#include "oldpreviewcontext.hpp"
#include "pickercirclecontext.hpp"
#include "resource.h"
#include "scolour.hpp"
#include "../TranslucentTB/util.hpp"

class GUI {
private:
	static const Util::string_view_map<const uint32_t> COLOR_MAP;
	static const std::tuple<const unsigned int, const unsigned int, const unsigned int> SLIDERS[8];
	static std::unordered_map<const COLORREF *, HWND> m_pickerMap;

	static INT_PTR CALLBACK ColourPickerDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

	using initdialog_pair_t = const std::pair<GUI *const, const COLORREF *const>;

	CColourPicker *m_picker;

	MainPickerContext m_pickerContext;
	PickerCircleContext m_circleContext;
	ColorSliderContext m_colorSliderContext;
	AlphaSliderContext m_alphaSliderContext;
	OldPreviewContext m_oldPreviewContext;
	NewPreviewContext m_newPreviewContext;
	const std::pair<RenderContext &, const unsigned int> m_contextPairs[6];

	bool m_changingText;
	bool m_changingHexViaSpin;
	bool m_initDone;
	HWND m_oldColorTip;
	HWND m_newColorTip;

	GUI(CColourPicker *picker, ID2D1Factory3 *factory, IDWriteFactory *dwFactory);

	INT_PTR OnDialogInit(HWND hDlg);
	INT_PTR OnDpiChange(HWND hDlg);
	INT_PTR OnSizeChange(HWND hDlg);
	INT_PTR OnDrawItem(HWND hDlg, LPARAM lParam);
	INT_PTR OnEraseBackground(HWND hDlg, WPARAM wParam);
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
	INT_PTR OnNonClientCalculateSize(HWND hDlg);
	INT_PTR OnWindowDestroy();

	static void InitHexInput(HWND hDlg);
	static HWND CreateTip(HWND hDlg, int item);
	static HRESULT CalculateDialogCoords(HWND hDlg, RECT &coords);

	HRESULT Redraw(HWND hDlg, bool skipMain = false, bool skipCircle = false,
		bool skipSlide = false, bool skipAlpha = false, bool skipNew = false, bool updateValues = true);
	HRESULT DrawItem(HWND hDlg, RenderContext &context, const SColourF &col);

	void UpdateValues(HWND hDlg);
	void ParseHex(HWND hDlg);
	static void FailedParse(HWND hDlg);

	static constexpr bool RectFitsInRect(const RECT &outer, const RECT &inner)
	{
		return inner.right <= outer.right && inner.left >= outer.left &&
			outer.top <= inner.top && outer.bottom >= inner.bottom;
	}

	static constexpr uint8_t ExpandOneLetterByte(uint8_t byte)
	{
		const uint8_t firstDigit = byte & 0xF;
		return (firstDigit << 4) + firstDigit;
	}

	inline GUI(const GUI &) = delete;
	inline GUI &operator =(const GUI &) = delete;

public:
	static HRESULT CreateGUI(CColourPicker *picker, COLORREF &value, HWND hParent);
};