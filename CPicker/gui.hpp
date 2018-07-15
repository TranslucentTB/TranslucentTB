#pragma once
#include "../TranslucentTB/arch.h"
#include <cstdint>
#include <unordered_map>
#include <windef.h>
#include <WinUser.h>

#include "ccolourpicker.hpp"
#include "scolour.hpp"
#include "../TranslucentTB/util.hpp"

class GUI {
private:
	static const Util::string_map<const uint32_t> COLOR_MAP;
	static const std::unordered_map<unsigned int, const std::pair<const unsigned int, const unsigned int>> SLIDER_MAP;
	static std::unordered_map<const uint32_t *, HWND> m_pickerMap;

	static INT_PTR CALLBACK ColourPickerDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK NoOutlineButtonSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR);
	static void UpdateValues(HWND hDlg, const SColour &col, bool &changing_text);
	static void FailedParse(HWND hDlg);
	static void ParseHex(HWND hDlg, CColourPicker *picker);

	inline static uint8_t ExpandOneLetterByte(const uint8_t &byte)
	{
		const uint8_t firstDigit = byte & 0xF;
		return (firstDigit << 4) + firstDigit;
	}

	class PaintContext {
	private:
		HWND m_handle;
		PAINTSTRUCT m_ps;

	public:
		inline PaintContext(HWND hWnd) : m_handle(hWnd)
		{
			BeginPaint(m_handle, &m_ps);
		}

		inline ~PaintContext()
		{
			EndPaint(m_handle, &m_ps);
		}

		inline PaintContext(const PaintContext &) = delete;
		inline PaintContext &operator =(const PaintContext &) = delete;
	};

public:
	static HRESULT CreateGUI(CColourPicker *picker, uint32_t &value, HWND hParent);
};